#include "ota_update.h"

#include "firmware_version.h"
#include "wifi_manager.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#include <cstring>
#include <cstdio>

namespace {
constexpr char kPrefsNamespace[] = "matrixsign";
constexpr char kOtaUrlKey[] = "otaUrl";
constexpr size_t kOtaUrlMax = 128;
constexpr size_t kRemoteBinUrlMax = 160;
constexpr size_t kMaxFirmwareBytes = 0x1E0000UL;
constexpr uint32_t kDownloadBufferSize = 4096;

char otaUrlBuf[kOtaUrlMax + 1]{};
OtaStatus status{};
bool uploadActive = false;
size_t uploadTotal = 0;
TaskHandle_t upgradeTaskHandle = nullptr;

bool parseVersion(const char *version, int parts[3]) {
  if (!version) {
    return false;
  }
  parts[0] = parts[1] = parts[2] = 0;
  return sscanf(version, "%d.%d.%d", &parts[0], &parts[1], &parts[2]) >= 1;
}

int compareVersions(const char *a, const char *b) {
  int av[3]{};
  int bv[3]{};
  parseVersion(a, av);
  parseVersion(b, bv);
  for (int i = 0; i < 3; i++) {
    if (av[i] != bv[i]) {
      return av[i] - bv[i];
    }
  }
  return 0;
}

void normalizeBaseUrl(char *url, size_t maxLen) {
  if (!url || url[0] == '\0') {
    return;
  }
  const size_t len = strlen(url);
  if (len > 0 && url[len - 1] != '/') {
    if (len + 1 < maxLen) {
      strcat(url, "/");
    }
  }
}

void setError(const char *message) {
  status.state = OtaState::Error;
  status.progress = 0;
  strlcpy(status.lastError, message ? message : "unknown error", sizeof(status.lastError));
}

void clearError() {
  status.lastError[0] = '\0';
}

void setHttpError(const char *context, int httpCode) {
  if (httpCode > 0) {
    snprintf(status.lastError, sizeof(status.lastError), "%s (HTTP %d)", context, httpCode);
  } else {
    strlcpy(status.lastError, context, sizeof(status.lastError));
  }
  status.state = OtaState::Error;
  status.progress = 0;
}

bool beginHttpClient(HTTPClient &http, const String &url, WiFiClient *plainClient,
                     WiFiClientSecure *secureClient) {
  if (url.startsWith("https://")) {
    secureClient->setInsecure();
    if (!http.begin(*secureClient, url)) {
      return false;
    }
  } else if (!http.begin(*plainClient, url)) {
    return false;
  }
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.setUserAgent("MatrixSign/" FIRMWARE_VERSION);
  http.addHeader("Cache-Control", "no-cache");
  http.addHeader("Pragma", "no-cache");
  return true;
}

struct ManifestFetchWork {
  char baseUrl[kOtaUrlMax + 1]{};
  char remoteVersion[16]{};
  char binUrl[kRemoteBinUrlMax + 1]{};
  char error[64]{};
  bool ok = false;
  SemaphoreHandle_t done = nullptr;
};

bool fetchVersionManifestWork(ManifestFetchWork *work) {
  if (!work) {
    return false;
  }

  if (!wifiStaConnected()) {
    strlcpy(work->error, "home Wi-Fi required", sizeof(work->error));
    return false;
  }

  char base[kOtaUrlMax + 2]{};
  strlcpy(base, work->baseUrl, sizeof(base));
  normalizeBaseUrl(base, sizeof(base));

  String manifestUrl = String(base) + "version.json?nc=" + String(millis());
  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  if (!beginHttpClient(http, manifestUrl, &plainClient, &secureClient)) {
    strlcpy(work->error, "manifest URL invalid", sizeof(work->error));
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    if (code > 0) {
      snprintf(work->error, sizeof(work->error), "manifest fetch failed (HTTP %d)", code);
    } else {
      strlcpy(work->error, "manifest fetch failed", sizeof(work->error));
    }
    return false;
  }

  JsonDocument doc;
  const String payload = http.getString();
  http.end();
  if (deserializeJson(doc, payload)) {
    strlcpy(work->error, "invalid manifest JSON", sizeof(work->error));
    return false;
  }

  const char *version = doc["version"] | "";
  if (version[0] == '\0') {
    strlcpy(work->error, "manifest missing version", sizeof(work->error));
    return false;
  }
  strlcpy(work->remoteVersion, version, sizeof(work->remoteVersion));

  if (!doc["url"].isNull()) {
    strlcpy(work->binUrl, doc["url"].as<const char *>(), sizeof(work->binUrl));
  } else {
    const char *binName = doc["bin"].isNull() ? "firmware.bin" : doc["bin"].as<const char *>();
    if (!binName || binName[0] == '\0') {
      binName = "firmware.bin";
    }
    String built = String(base) + binName;
    strlcpy(work->binUrl, built.c_str(), sizeof(work->binUrl));
  }
  return true;
}

void manifestFetchTask(void *param) {
  ManifestFetchWork *work = static_cast<ManifestFetchWork *>(param);
  work->ok = fetchVersionManifestWork(work);
  if (work->done != nullptr) {
    xSemaphoreGive(work->done);
  }
  vTaskDelete(nullptr);
}

bool fetchVersionManifest(const char *baseUrl, char *remoteVersion, size_t versionLen,
                          char *binUrlOut, size_t binUrlLen) {
  ManifestFetchWork work{};
  strlcpy(work.baseUrl, baseUrl, sizeof(work.baseUrl));
  work.done = xSemaphoreCreateBinary();
  if (work.done == nullptr) {
    setError("out of memory");
    return false;
  }

  if (xTaskCreate(manifestFetchTask, "ota_check", 8192, &work, 1, nullptr) != pdPASS) {
    vSemaphoreDelete(work.done);
    setError("check task failed");
    return false;
  }

  if (xSemaphoreTake(work.done, pdMS_TO_TICKS(20000)) != pdTRUE) {
    vSemaphoreDelete(work.done);
    setError("manifest fetch timeout");
    return false;
  }
  vSemaphoreDelete(work.done);

  if (!work.ok) {
    setError(work.error[0] != '\0' ? work.error : "manifest fetch failed");
    return false;
  }

  strlcpy(remoteVersion, work.remoteVersion, versionLen);
  strlcpy(binUrlOut, work.binUrl, binUrlLen);
  return true;
}

bool flashFromStream(Stream &stream, size_t contentLength) {
  if (contentLength > kMaxFirmwareBytes) {
    setError("firmware too large");
    return false;
  }

  status.state = OtaState::Flashing;
  status.progress = 0;

  if (!Update.begin(contentLength == 0 ? UPDATE_SIZE_UNKNOWN : contentLength, U_FLASH)) {
    setError(Update.errorString());
    return false;
  }

  uint8_t buffer[kDownloadBufferSize];
  size_t written = 0;
  while (stream.available() || (contentLength > 0 && written < contentLength)) {
    const size_t n = stream.readBytes(buffer, sizeof(buffer));
    if (n == 0) {
      if (!stream.available()) {
        break;
      }
      delay(1);
      continue;
    }
    if (Update.write(buffer, n) != n) {
      Update.abort();
      setError("flash write failed");
      return false;
    }
    written += n;
    if (contentLength > 0) {
      status.progress = static_cast<uint8_t>((written * 100UL) / contentLength);
    }
  }

  if (!Update.end(true)) {
    setError(Update.errorString());
    return false;
  }

  status.progress = 100;
  status.state = OtaState::Idle;
  return true;
}

void upgradeTask(void *param) {
  char *url = static_cast<char *>(param);
  status.state = OtaState::Downloading;
  status.progress = 0;
  clearError();

  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  if (!beginHttpClient(http, url, &plainClient, &secureClient)) {
    setError("download URL invalid");
    free(url);
    upgradeTaskHandle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    setHttpError("download failed", code);
    free(url);
    upgradeTaskHandle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  const int contentLength = http.getSize();
  WiFiClient *stream = http.getStreamPtr();
  const bool ok = flashFromStream(*stream, contentLength > 0 ? static_cast<size_t>(contentLength) : 0);
  http.end();
  free(url);

  if (ok) {
    delay(250);
    ESP.restart();
  }

  upgradeTaskHandle = nullptr;
  vTaskDelete(nullptr);
}
}  // namespace

void otaUpdateBegin() {
  strlcpy(otaUrlBuf, FIRMWARE_OTA_URL_DEFAULT, sizeof(otaUrlBuf));
  normalizeBaseUrl(otaUrlBuf, sizeof(otaUrlBuf));

  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  if (prefs.isKey(kOtaUrlKey)) {
    prefs.getString(kOtaUrlKey, otaUrlBuf, sizeof(otaUrlBuf));
    normalizeBaseUrl(otaUrlBuf, sizeof(otaUrlBuf));
  }
  prefs.end();

  status = OtaStatus{};
  uploadActive = false;
  uploadTotal = 0;
}

const char *otaUpdateVersion() {
  return FIRMWARE_VERSION;
}

const char *otaUpdateUrl() {
  return otaUrlBuf;
}

bool otaUpdateSetUrl(const char *url) {
  if (!url || url[0] == '\0' || strlen(url) >= kOtaUrlMax) {
    return false;
  }
  strlcpy(otaUrlBuf, url, sizeof(otaUrlBuf));
  normalizeBaseUrl(otaUrlBuf, sizeof(otaUrlBuf));

  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putString(kOtaUrlKey, otaUrlBuf);
  prefs.end();
  return true;
}

OtaStatus otaUpdateStatus() {
  return status;
}

bool otaUpdateIsActive() {
  return uploadActive || upgradeTaskHandle != nullptr || status.state == OtaState::Downloading ||
         status.state == OtaState::Flashing;
}

const char *otaUpdateStateString(OtaState state) {
  switch (state) {
    case OtaState::Checking:
      return "checking";
    case OtaState::Downloading:
      return "downloading";
    case OtaState::Flashing:
      return "flashing";
    case OtaState::Error:
      return "error";
    default:
      return "idle";
  }
}

bool otaUpdateCheckRemote() {
  if (upgradeTaskHandle != nullptr || uploadActive) {
    setError("update in progress");
    return false;
  }

  status.state = OtaState::Checking;
  status.progress = 0;
  status.updateAvailable = false;
  status.remoteVersion[0] = '\0';
  status.remoteBinUrl[0] = '\0';
  clearError();

  char remoteVersion[sizeof(status.remoteVersion)]{};
  char binUrl[kRemoteBinUrlMax + 1]{};
  if (!fetchVersionManifest(otaUrlBuf, remoteVersion, sizeof(remoteVersion), binUrl,
                            sizeof(binUrl))) {
    return false;
  }

  strlcpy(status.remoteVersion, remoteVersion, sizeof(status.remoteVersion));
  strlcpy(status.remoteBinUrl, binUrl, sizeof(status.remoteBinUrl));
  status.updateAvailable = compareVersions(remoteVersion, FIRMWARE_VERSION) > 0;
  status.state = OtaState::Idle;
  clearError();
  return true;
}

bool otaUpdateStartUpgrade(const char *urlOverride) {
  if (upgradeTaskHandle != nullptr || uploadActive) {
    setError("update in progress");
    return false;
  }
  if (!wifiStaConnected()) {
    setError("home Wi-Fi required");
    return false;
  }

  const char *sourceUrl = urlOverride;
  if (!sourceUrl || sourceUrl[0] == '\0') {
    if (status.remoteBinUrl[0] == '\0') {
      if (!otaUpdateCheckRemote()) {
        return false;
      }
    }
    if (!status.updateAvailable && status.remoteBinUrl[0] != '\0') {
      // Allow explicit upgrade even if version matches (reinstall).
    }
    sourceUrl = status.remoteBinUrl;
  }

  if (!sourceUrl || sourceUrl[0] == '\0') {
    setError("no firmware URL");
    return false;
  }

  char *urlCopy = static_cast<char *>(malloc(strlen(sourceUrl) + 1));
  if (!urlCopy) {
    setError("out of memory");
    return false;
  }
  strcpy(urlCopy, sourceUrl);

  clearError();
  if (xTaskCreate(upgradeTask, "ota_upgrade", 8192, urlCopy, 1, &upgradeTaskHandle) != pdPASS) {
    free(urlCopy);
    upgradeTaskHandle = nullptr;
    setError("task start failed");
    return false;
  }
  return true;
}

bool otaUpdateWriteChunk(const uint8_t *data, size_t len, size_t index, size_t total, bool final) {
  if (upgradeTaskHandle != nullptr) {
    setError("remote update in progress");
    return false;
  }

  if (index == 0) {
    uploadActive = true;
    uploadTotal = total;
    status.state = OtaState::Flashing;
    status.progress = 0;
    clearError();

    if (total > kMaxFirmwareBytes) {
      uploadActive = false;
      setError("firmware too large");
      return false;
    }

    const size_t beginSize = total > 0 ? total : UPDATE_SIZE_UNKNOWN;
    if (!Update.begin(beginSize, U_FLASH)) {
      uploadActive = false;
      setError(Update.errorString());
      return false;
    }
  }

  if (!uploadActive) {
    return false;
  }

  if (Update.write(const_cast<uint8_t *>(data), len) != len) {
    Update.abort();
    uploadActive = false;
    setError("flash write failed");
    return false;
  }

  const size_t written = index + len;
  if (total > 0) {
    status.progress = static_cast<uint8_t>((written * 100UL) / total);
  } else if (final) {
    status.progress = 100;
  }

  if (final && (index + len) > kMaxFirmwareBytes) {
    Update.abort();
    uploadActive = false;
    setError("firmware too large");
    return false;
  }

  if (final) {
    if (!Update.end(true)) {
      uploadActive = false;
      setError(Update.errorString());
      return false;
    }
    uploadActive = false;
    status.progress = 100;
    status.state = OtaState::Idle;
    delay(250);
    ESP.restart();
  }

  return true;
}

void otaUpdateAbortUpload() {
  if (uploadActive) {
    Update.abort();
    uploadActive = false;
  }
  if (status.state == OtaState::Flashing && upgradeTaskHandle == nullptr) {
    status.state = OtaState::Idle;
    status.progress = 0;
  }
}
