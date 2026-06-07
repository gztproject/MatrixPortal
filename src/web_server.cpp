#include "web_server.h"

#include "effect_renderer.h"
#include "web_ui.h"
#include "wifi_manager.h"
#include "time_sync.h"

#include "web_auth.h"
#include "ota_update.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>

namespace {
AsyncWebServer server(80);
DisplayEngine *displayEngine = nullptr;
PresetStore *presetStore = nullptr;

File uploadFile;
int uploadPresetId = -1;
size_t uploadTotalBytes = 0;

#define AUTH(request)           \
  do {                          \
    if (!webAuthCheck(request)) \
      return;                   \
  } while (0)

#define AUTH_BODY(request, index) \
  do {                            \
    if ((index) == 0 && !webAuthCheck(request)) \
      return;                     \
  } while (0)

String *accumulateRequestBody(AsyncWebServerRequest *request, uint8_t *data, size_t len,
                              size_t index, size_t total) {
  if (index == 0) {
    request->_tempObject = new String();
    static_cast<String *>(request->_tempObject)->reserve(total + 1);
  }
  auto *body = static_cast<String *>(request->_tempObject);
  if (body == nullptr) {
    return nullptr;
  }
  body->concat(reinterpret_cast<const char *>(data), len);
  if (index + len < total) {
    return nullptr;
  }
  return body;
}

void releaseRequestBody(AsyncWebServerRequest *request) {
  if (request->_tempObject != nullptr) {
    delete static_cast<String *>(request->_tempObject);
    request->_tempObject = nullptr;
  }
}

void jsonAssignString(JsonObject obj, const char *key, char *dest, size_t destSize) {
  JsonVariantConst value = obj[key];
  if (value.isNull()) {
    return;
  }
  if (value.is<const char *>()) {
    strlcpy(dest, value.as<const char *>(), destSize);
    return;
  }
  const String text = value.as<String>();
  strlcpy(dest, text.c_str(), destSize);
}

void sendJsonResponse(AsyncWebServerRequest *request, int code, const String &body) {
  AsyncWebServerResponse *response = request->beginResponse(code, "application/json", body);
  response->addHeader("Cache-Control", "no-store");
  response->addHeader("Pragma", "no-cache");
  request->send(response);
}

uint32_t parseHexColor(const char *hex) {
  if (!hex || hex[0] != '#') {
    return 0xFFFFFF;
  }
  return strtoul(hex + 1, nullptr, 16) & 0xFFFFFF;
}

void appendWifiStatus(JsonObject obj) {
  obj["apSsid"] = wifiApSsid();
  obj["apIp"] = wifiApIp();
  obj["staConnected"] = wifiStaConnected();
  if (wifiStaConnected()) {
    obj["staIp"] = wifiStaIp();
    obj["staRssi"] = wifiStaRssi();
  } else {
    obj["staIp"] = "";
    obj["staRssi"] = nullptr;
  }
}

void appendGlobalBrightness(JsonObject obj) {
  obj["brightness"] = presetStore->globalBrightness();
}

void presetToJson(const SignPreset &preset, JsonObject obj, int index) {
  obj["contentType"] = contentTypeToString(preset.contentType);
  obj["effectId"] = effectIdToString(static_cast<EffectId>(preset.effectId));
  obj["effectLabel"] = effectLabel(static_cast<EffectId>(preset.effectId));
  obj["textHeightPx"] = preset.textHeightPx;
  obj["rowCount"] = preset.rowCount;
  obj["text"] = preset.text;
  obj["label"] = preset.label;
  obj["scroll"] = preset.scroll;
  obj["scrollDelayMs"] = preset.scrollDelayMs;
  obj["effectParam"] = preset.effectParam;
  obj["countdownEndUnix"] = preset.countdownEndUnix;
  obj["countdownDurationSec"] = preset.countdownDurationSec;
  obj["contentOffsetX"] = preset.contentOffsetX;
  obj["contentOffsetY"] = preset.contentOffsetY;

  char color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", preset.colorR, preset.colorG, preset.colorB);
  obj["color"] = color;
  obj["gifPath"] = preset.gifPath;
  obj["hasGif"] = presetStore->gifExistsForSlot(index);
}

void appendPlaylistFields(JsonObject obj) {
  const PlaylistSettings playlist = presetStore->playlist();
  obj["playlistEnabled"] = playlist.enabled;
  obj["playlistMask"] = playlist.slotMask;
  obj["playlistDwellMs"] = playlist.dwellMs;
}

void appendTimezoneFields(JsonObject obj) {
  obj["timezoneId"] = presetStore->timezoneId();
}

void appendTimeFields(JsonObject obj) {
  obj["timeValid"] = timeIsValid();
  obj["timeSource"] = timeSyncSourceString(timeSyncSource());
}

void appendFirmwareFields(JsonObject obj) {
  const OtaStatus ota = otaUpdateStatus();
  obj["firmwareVersion"] = otaUpdateVersion();
  obj["otaUrl"] = otaUpdateUrl();
  obj["otaState"] = otaUpdateStateString(ota.state);
  obj["otaProgress"] = ota.progress;
  obj["otaUpdateAvailable"] = ota.updateAvailable;
  if (ota.remoteVersion[0] != '\0') {
    obj["otaRemoteVersion"] = ota.remoteVersion;
  }
  if (ota.lastError[0] != '\0') {
    obj["otaError"] = ota.lastError;
  }
}

void appendOtaStatusJson(JsonObject obj) {
  const OtaStatus ota = otaUpdateStatus();
  obj["version"] = otaUpdateVersion();
  obj["otaUrl"] = otaUpdateUrl();
  obj["state"] = otaUpdateStateString(ota.state);
  obj["progress"] = ota.progress;
  obj["updateAvailable"] = ota.updateAvailable;
  if (ota.remoteVersion[0] != '\0') {
    obj["remoteVersion"] = ota.remoteVersion;
  }
  if (ota.remoteBinUrl[0] != '\0') {
    obj["remoteBinUrl"] = ota.remoteBinUrl;
  }
  if (ota.lastError[0] != '\0') {
    obj["error"] = ota.lastError;
  }
}

bool jsonToPreset(JsonObject obj, SignPreset &preset) {
  if (obj["contentType"].is<const char *>()) {
    preset.contentType = contentTypeFromString(obj["contentType"]);
  }
  if (obj["effectId"].is<const char *>()) {
    preset.effectId = static_cast<uint8_t>(effectIdFromString(obj["effectId"]));
  } else if (obj["effectId"].is<int>()) {
    preset.effectId = static_cast<uint8_t>(obj["effectId"].as<int>());
  }
  if (obj["textHeightPx"].is<int>()) {
    preset.textHeightPx = clampTextHeightPx(obj["textHeightPx"].as<int>());
  } else if (obj["fontScale"].is<const char *>()) {
    preset.textHeightPx = textHeightPxFromLegacyFontScale(obj["fontScale"]);
  }
  if (obj["rowCount"].is<int>()) {
    preset.rowCount = static_cast<uint8_t>(obj["rowCount"].as<int>());
  } else if (obj["rowCount"].is<uint8_t>()) {
    preset.rowCount = obj["rowCount"];
  }
  jsonAssignString(obj, "text", preset.text, sizeof(preset.text));
  jsonAssignString(obj, "label", preset.label, sizeof(preset.label));
  if (obj["scroll"].is<bool>()) {
    preset.scroll = obj["scroll"];
  }
  if (obj["scrollDelayMs"].is<uint16_t>()) {
    preset.scrollDelayMs = obj["scrollDelayMs"];
  } else if (obj["scrollDelayMs"].is<int>()) {
    preset.scrollDelayMs = static_cast<uint16_t>(obj["scrollDelayMs"].as<int>());
  }
  if (obj["color"].is<const char *>()) {
    const uint32_t rgb = parseHexColor(obj["color"]);
    preset.colorR = (rgb >> 16) & 0xFF;
    preset.colorG = (rgb >> 8) & 0xFF;
    preset.colorB = rgb & 0xFF;
  }
  if (obj["effectParam"].is<int>()) {
    preset.effectParam = static_cast<uint8_t>(obj["effectParam"].as<int>());
  }
  if (obj["countdownEndUnix"].is<uint32_t>()) {
    preset.countdownEndUnix = obj["countdownEndUnix"];
  } else if (obj["countdownEndUnix"].is<int>()) {
    preset.countdownEndUnix = static_cast<uint32_t>(obj["countdownEndUnix"].as<int>());
  }
  if (obj["countdownDurationSec"].is<uint32_t>()) {
    preset.countdownDurationSec = obj["countdownDurationSec"];
  } else if (obj["countdownDurationSec"].is<int>()) {
    preset.countdownDurationSec = static_cast<uint32_t>(obj["countdownDurationSec"].as<int>());
  }
  if (obj["contentOffsetX"].is<int>()) {
    preset.contentOffsetX = clampContentOffset(obj["contentOffsetX"].as<int>());
  }
  if (obj["contentOffsetY"].is<int>()) {
    preset.contentOffsetY = clampContentOffset(obj["contentOffsetY"].as<int>());
  }
  if (preset.rowCount < 1) {
    preset.rowCount = 1;
  }
  if (preset.rowCount > 4) {
    preset.rowCount = 4;
  }
  return true;
}

void sendPresetsJson(AsyncWebServerRequest *request) {
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  root["activeIndex"] = presetStore->activeIndex();
  JsonArray arr = root["presets"].to<JsonArray>();
  for (int i = 0; i < PRESET_COUNT; i++) {
    JsonObject item = arr.add<JsonObject>();
    presetToJson(presetStore->get(i), item, i);
  }
  appendGlobalBrightness(root);
  appendPlaylistFields(root);
  appendTimezoneFields(root);
  appendTimeFields(root);
  appendFirmwareFields(root);
  appendWifiStatus(root);

  if (doc.overflowed()) {
    sendJsonResponse(request, 500, "{\"error\":\"json overflow\"}");
    return;
  }

  String body;
  body.reserve(4096);
  serializeJson(doc, body);
  sendJsonResponse(request, 200, body);
}

void appendPresetFields(JsonObject obj, const SignPreset &preset) {
  obj["contentType"] = contentTypeToString(preset.contentType);
  obj["effectId"] = effectIdToString(static_cast<EffectId>(preset.effectId));
  obj["effectLabel"] = effectLabel(static_cast<EffectId>(preset.effectId));
  obj["textHeightPx"] = preset.textHeightPx;
  obj["rowCount"] = preset.rowCount;
  obj["text"] = preset.text;
  obj["label"] = preset.label;
  obj["scroll"] = preset.scroll;
  obj["scrollDelayMs"] = preset.scrollDelayMs;
  obj["effectParam"] = preset.effectParam;
  obj["countdownEndUnix"] = preset.countdownEndUnix;
  obj["countdownDurationSec"] = preset.countdownDurationSec;
  obj["contentOffsetX"] = preset.contentOffsetX;
  obj["contentOffsetY"] = preset.contentOffsetY;
  char color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", preset.colorR, preset.colorG, preset.colorB);
  obj["color"] = color;
}
}  // namespace

void webServerBegin(DisplayEngine &engine, PresetStore &store) {
  displayEngine = &engine;
  presetStore = &store;

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AUTH(request);
    request->send_P(200, "text/html", WEB_UI_HTML);
  });

  server.on("/api/effects", HTTP_GET, [](AsyncWebServerRequest *request) {
    AUTH(request);
    JsonDocument doc;
    JsonArray arr = doc["effects"].to<JsonArray>();
    size_t count = 0;
    const EffectInfo *catalog = effectCatalog(&count);
    for (size_t i = 0; i < count; i++) {
      JsonObject item = arr.add<JsonObject>();
      item["id"] = catalog[i].id;
      item["label"] = catalog[i].label;
      item["monochrome"] = catalog[i].monochrome;
    }
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/presets/select", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const int id = doc["id"] | -1;
              if (id < 0 || id >= PRESET_COUNT) {
                request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
                return;
              }
              displayEngine->selectPreset(id);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/brightness", HTTP_GET, [](AsyncWebServerRequest *request) {
    AUTH(request);
    JsonDocument doc;
    doc["brightness"] = presetStore->globalBrightness();
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const int brightness = doc["brightness"] | -1;
              if (brightness < 1 || brightness > 100) {
                request->send(400, "application/json", "{\"error\":\"brightness must be 1-100\"}");
                return;
              }
              displayEngine->setGlobalBrightness(static_cast<uint8_t>(brightness));
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/presets", HTTP_GET, [](AsyncWebServerRequest *request) {
    AUTH(request);
    sendPresetsJson(request);
  });

  server.on(AsyncURIMatcher::exact("/api/presets"), HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              String *body = accumulateRequestBody(request, data, len, index, total);
              if (body == nullptr) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, body->c_str()) || doc.overflowed()) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const int id = doc["id"] | presetStore->activeIndex();
              if (id < 0 || id >= PRESET_COUNT) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
                return;
              }
              SignPreset preset = presetStore->get(id);
              if (!jsonToPreset(doc.as<JsonObject>(), preset)) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid preset\"}");
                return;
              }
              displayEngine->applyPreset(preset, id);
              releaseRequestBody(request);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    AUTH(request);
    const SignPreset preset = displayEngine->activePreset();
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    appendPresetFields(root, preset);
    root["activeIndex"] = displayEngine->activeIndex();
    appendGlobalBrightness(root);
    appendWifiStatus(root);

    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              SignPreset preset = displayEngine->activePreset();
              if (!jsonToPreset(doc.as<JsonObject>(), preset)) {
                request->send(400, "application/json", "{\"error\":\"invalid config\"}");
                return;
              }
              displayEngine->applyPreset(preset, displayEngine->activeIndex());
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/preview", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              String *body = accumulateRequestBody(request, data, len, index, total);
              if (body == nullptr) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, body->c_str())) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const int slot = doc["slot"] | doc["id"] | 0;
              if (slot < 0 || slot >= PRESET_COUNT) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid slot\"}");
                return;
              }
              SignPreset preset = presetStore->get(slot);
              if (!jsonToPreset(doc.as<JsonObject>(), preset)) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid preview\"}");
                return;
              }
              displayEngine->previewOnPanel(preset, slot);
              releaseRequestBody(request);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/presets/duplicate", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const int from = doc["from"] | -1;
              const int to = doc["to"] | -1;
              if (from < 0 || from >= PRESET_COUNT || to < 0 || to >= PRESET_COUNT) {
                request->send(400, "application/json", "{\"error\":\"invalid slot\"}");
                return;
              }
              if (!presetStore->duplicateSlot(from, to)) {
                request->send(500, "application/json", "{\"error\":\"duplicate failed\"}");
                return;
              }
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/playlist", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              PlaylistSettings playlist = presetStore->playlist();
              if (doc["enabled"].is<bool>()) {
                playlist.enabled = doc["enabled"];
              }
              if (doc["slotMask"].is<int>()) {
                playlist.slotMask = static_cast<uint8_t>(doc["slotMask"].as<int>() & 0xFF);
              }
              if (doc["dwellMs"].is<int>()) {
                playlist.dwellMs = clampPlaylistDwellMs(doc["dwellMs"].as<int>());
              }
              presetStore->setPlaylist(playlist);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/timezone", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              String *body = accumulateRequestBody(request, data, len, index, total);
              if (body == nullptr) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, body->c_str())) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const char *timezoneId = doc["timezoneId"] | timeSyncDefaultTimezoneId();
              if (!timeSyncIsKnownTimezoneId(timezoneId)) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid timezone\"}");
                return;
              }
              presetStore->setTimezoneId(timezoneId);
              displayEngine->refreshTimeDisplay();
              releaseRequestBody(request);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/time/sync", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              time_t unixSeconds = 0;
              if (doc["unix"].is<int>()) {
                unixSeconds = doc["unix"].as<time_t>();
              } else if (doc["unix"].is<uint32_t>()) {
                unixSeconds = static_cast<time_t>(doc["unix"].as<uint32_t>());
              } else if (doc["unixSeconds"].is<int>()) {
                unixSeconds = doc["unixSeconds"].as<time_t>();
              }
              if (!timeSyncSetUnix(unixSeconds)) {
                request->send(400, "application/json", "{\"error\":\"invalid time\"}");
                return;
              }
              displayEngine->refreshTimeDisplay();
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/auth/password", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const char *password = doc["password"] | "";
              if (!webAuthSetPassword(password)) {
                request->send(400, "application/json",
                              "{\"error\":\"password must be at least 8 characters\"}");
                return;
              }
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
    AUTH(request);
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["activeIndex"] = presetStore->activeIndex();
    appendGlobalBrightness(root);
    appendPlaylistFields(root);
    appendTimezoneFields(root);
    appendTimeFields(root);
    JsonArray arr = root["presets"].to<JsonArray>();
    for (int i = 0; i < PRESET_COUNT; i++) {
      JsonObject item = arr.add<JsonObject>();
      presetToJson(presetStore->get(i), item, i);
    }
    if (doc.overflowed()) {
      sendJsonResponse(request, 500, "{\"error\":\"json overflow\"}");
      return;
    }
    String body;
    serializeJson(doc, body);
    sendJsonResponse(request, 200, body);
  });

  server.on("/api/restore", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              String *body = accumulateRequestBody(request, data, len, index, total);
              if (body == nullptr) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, body->c_str())) {
                releaseRequestBody(request);
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              if (doc["brightness"].is<int>()) {
                displayEngine->setGlobalBrightness(
                    static_cast<uint8_t>(doc["brightness"].as<int>()));
              }
              if (doc["playlistEnabled"].is<bool>() || doc["slotMask"].is<int>() ||
                  doc["dwellMs"].is<int>() || doc["playlistDwellMs"].is<int>()) {
                PlaylistSettings playlist = presetStore->playlist();
                if (doc["playlistEnabled"].is<bool>()) {
                  playlist.enabled = doc["playlistEnabled"];
                }
                if (doc["slotMask"].is<int>()) {
                  playlist.slotMask = static_cast<uint8_t>(doc["slotMask"].as<int>() & 0xFF);
                }
                int dwell = 0;
                if (doc["dwellMs"].is<int>()) {
                  dwell = doc["dwellMs"].as<int>();
                } else if (doc["playlistDwellMs"].is<int>()) {
                  dwell = doc["playlistDwellMs"].as<int>();
                }
                if (dwell > 0) {
                  playlist.dwellMs = clampPlaylistDwellMs(dwell);
                }
                presetStore->setPlaylist(playlist);
              }
              if (!doc["timezoneId"].isNull()) {
                char tz[TIMEZONE_ID_MAX + 1]{};
                strlcpy(tz, doc["timezoneId"].as<const char *>(), sizeof(tz));
                if (timeSyncIsKnownTimezoneId(tz)) {
                  presetStore->setTimezoneId(tz);
                }
              }
              JsonArray arr = doc["presets"].as<JsonArray>();
              if (!arr.isNull()) {
                int i = 0;
                for (JsonObject item : arr) {
                  if (i >= PRESET_COUNT) {
                    break;
                  }
                  SignPreset preset = presetStore->get(i);
                  jsonToPreset(item, preset);
                  presetStore->set(i, preset);
                  i++;
                }
              }
              displayEngine->selectPreset(presetStore->activeIndex());
              displayEngine->refreshTimeDisplay();
              releaseRequestBody(request);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    AUTH(request);
    wifiManagerForgetSta();
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const char *ssid = doc["ssid"] | "";
              const char *password = doc["password"] | "";
              if (ssid[0] == '\0') {
                request->send(400, "application/json", "{\"error\":\"ssid required\"}");
                return;
              }
              wifiManagerConnectSta(ssid, password);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/presets/gif", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    AUTH(request);
    const int id = request->hasParam("id") ? request->getParam("id")->value().toInt() : -1;
    if (id < 0 || id >= PRESET_COUNT) {
      request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
      return;
    }
    const String path = presetStore->gifPathForSlot(id);
    if (LittleFS.exists(path)) {
      LittleFS.remove(path);
    }
    SignPreset preset = presetStore->get(id);
    path.toCharArray(preset.gifPath, sizeof(preset.gifPath));
    if (preset.contentType == ContentType::Gif) {
      preset.contentType = ContentType::Text;
    }
    displayEngine->applyPreset(preset, id);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on(
      "/api/presets/gif",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {
        AUTH(request);
        if (uploadPresetId < 0 || uploadPresetId >= PRESET_COUNT) {
          request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
          return;
        }
        if (uploadTotalBytes > MAX_GIF_BYTES) {
          const String path = presetStore->gifPathForSlot(uploadPresetId);
          if (LittleFS.exists(path)) {
            LittleFS.remove(path);
          }
          request->send(413, "application/json", "{\"error\":\"gif too large (max 256KB)\"}");
          return;
        }
        SignPreset preset = presetStore->get(uploadPresetId);
        const String path = presetStore->gifPathForSlot(uploadPresetId);
        path.toCharArray(preset.gifPath, sizeof(preset.gifPath));
        preset.contentType = ContentType::Gif;
        displayEngine->applyPreset(preset, uploadPresetId);
        request->send(200, "application/json", "{\"ok\":true}");
      },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len,
         bool final) {
        if (index == 0) {
          AUTH_BODY(request, index);
          uploadPresetId = request->hasParam("id") ? request->getParam("id")->value().toInt() : -1;
          uploadTotalBytes = 0;
          if (uploadPresetId < 0 || uploadPresetId >= PRESET_COUNT) {
            return;
          }
          const String path = presetStore->gifPathForSlot(uploadPresetId);
          if (LittleFS.exists(path)) {
            LittleFS.remove(path);
          }
          uploadFile = LittleFS.open(path, "w");
        }

        if (!uploadFile) {
          return;
        }

        uploadTotalBytes += len;
        if (uploadTotalBytes > MAX_GIF_BYTES) {
          uploadFile.close();
          const String path = presetStore->gifPathForSlot(uploadPresetId);
          LittleFS.remove(path);
          return;
        }

        uploadFile.write(data, len);
        if (final) {
          uploadFile.close();
        }
      });

  server.on("/api/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
    AUTH(request);
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    appendOtaStatusJson(root);
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/firmware/url", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const char *otaUrl = doc["otaUrl"] | "";
              if (!otaUpdateSetUrl(otaUrl)) {
                request->send(400, "application/json", "{\"error\":\"invalid ota url\"}");
                return;
              }
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/firmware/check", HTTP_POST, [](AsyncWebServerRequest *request) {
    AUTH(request);
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    const bool checked = otaUpdateCheckRemote();
    root["ok"] = checked;
    appendOtaStatusJson(root);
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/firmware/upgrade", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              AUTH_BODY(request, index);
              if (index + len != total) {
                return;
              }
              const char *urlOverride = nullptr;
              if (len > 0) {
                JsonDocument doc;
                if (!deserializeJson(doc, data, len)) {
                  request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                  return;
                }
                if (doc["url"].is<const char *>()) {
                  urlOverride = doc["url"];
                }
              }
              if (!otaUpdateStartUpgrade(urlOverride)) {
                JsonDocument doc;
                JsonObject root = doc.to<JsonObject>();
                root["ok"] = false;
                appendOtaStatusJson(root);
                String body;
                serializeJson(doc, body);
                request->send(400, "application/json", body);
                return;
              }
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on(
      "/api/firmware/upload",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {
        AUTH(request);
        const OtaStatus ota = otaUpdateStatus();
        if (ota.state == OtaState::Error) {
          JsonDocument doc;
          JsonObject root = doc.to<JsonObject>();
          root["ok"] = false;
          appendOtaStatusJson(root);
          String body;
          serializeJson(doc, body);
          request->send(400, "application/json", body);
          return;
        }
        request->send(200, "application/json", "{\"ok\":true}");
      },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len,
         bool final) {
        if (index == 0) {
          AUTH_BODY(request, index);
          otaUpdateAbortUpload();
        }
        if (!otaUpdateWriteChunk(data, len, index, final ? index + len : 0, final)) {
          return;
        }
        (void)filename;
      });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Web server started on port 80");
}
