#include "preset_store.h"

#include "effect_renderer.h"
#include "panel_profile.h"
#include "time_sync.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <cstring>
#include <freertos/semphr.h>

namespace {
constexpr char kNs[] = "presets";
constexpr uint8_t kDefaultBrightness = 10;
constexpr size_t kPresetJsonMax = 1024;

SemaphoreHandle_t storeMutex = nullptr;

class StoreLock {
 public:
  StoreLock() {
    if (storeMutex != nullptr) {
      xSemaphoreTakeRecursive(storeMutex, portMAX_DELAY);
    }
  }
  ~StoreLock() {
    if (storeMutex != nullptr) {
      xSemaphoreGiveRecursive(storeMutex);
    }
  }
};

void copyJsonStringValue(JsonVariantConst value, char *dest, size_t destSize) {
  if (value.isNull() || destSize == 0) {
    return;
  }
  if (value.is<const char *>()) {
    strlcpy(dest, value.as<const char *>(), destSize);
    return;
  }
  const JsonString parsed = value.as<JsonString>();
  if (!parsed.isNull()) {
    strlcpy(dest, parsed.c_str(), destSize);
  }
}

void setJsonStringMember(JsonObject obj, const char *key, const char *value) {
  obj[key].set(value != nullptr ? value : "");
}

bool buildPresetJson(const SignPreset &preset, char *out, size_t outSize) {
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  setJsonStringMember(root, "contentType", contentTypeToString(preset.contentType));
  setJsonStringMember(root, "effectId", effectIdToString(static_cast<EffectId>(preset.effectId)));
  root["textHeightPx"] = preset.textHeightPx;
  root["rowCount"] = preset.rowCount;
  setJsonStringMember(root, "message", preset.message);
  setJsonStringMember(root, "slotLabel", preset.label);
  root["scroll"] = preset.scroll;
  root["scrollDelayMs"] = preset.scrollDelayMs;
  root["colorR"] = preset.colorR;
  root["colorG"] = preset.colorG;
  root["colorB"] = preset.colorB;
  root["effectParam"] = preset.effectParam;
  root["countdownEndUnix"] = preset.countdownEndUnix;
  root["countdownDurationSec"] = preset.countdownDurationSec;
  root["contentOffsetX"] = preset.contentOffsetX;
  root["contentOffsetY"] = preset.contentOffsetY;
  setJsonStringMember(root, "gifPath", preset.gifPath);

  if (doc.overflowed()) {
    return false;
  }

  const size_t n = serializeJson(doc, out, outSize);
  return n > 0 && n < outSize;
}
}  // namespace

const char *contentTypeToString(ContentType type) {
  switch (type) {
    case ContentType::Gif:
      return "gif";
    case ContentType::Effect:
      return "effect";
    case ContentType::Clock:
      return "clock";
    case ContentType::Countdown:
      return "countdown";
    case ContentType::Text:
    default:
      return "text";
  }
}

ContentType contentTypeFromString(const char *value) {
  if (!value) {
    return ContentType::Text;
  }
  if (strcmp(value, "gif") == 0) {
    return ContentType::Gif;
  }
  if (strcmp(value, "effect") == 0) {
    return ContentType::Effect;
  }
  if (strcmp(value, "clock") == 0) {
    return ContentType::Clock;
  }
  if (strcmp(value, "countdown") == 0) {
    return ContentType::Countdown;
  }
  return ContentType::Text;
}

uint8_t clampTextHeightPx(int px) {
  if (px < TEXT_HEIGHT_PX_MIN) {
    return TEXT_HEIGHT_PX_MIN;
  }
  if (px > PANEL_RES_Y) {
    return PANEL_RES_Y;
  }
  return static_cast<uint8_t>(px);
}

uint8_t textHeightPxFromLegacyFontScale(const char *value) {
  if (!value) {
    return TEXT_HEIGHT_PX_DEFAULT;
  }
  if (strcmp(value, "quarter") == 0) {
    return clampTextHeightPx(PANEL_RES_Y / 4);
  }
  if (strcmp(value, "three_quarter") == 0) {
    return clampTextHeightPx((PANEL_RES_Y * 3) / 4);
  }
  if (strcmp(value, "full") == 0) {
    return clampTextHeightPx(PANEL_RES_Y);
  }
  return TEXT_HEIGHT_PX_DEFAULT;
}

uint8_t clampBrightness(int value) {
  if (value < 1) {
    return 1;
  }
  if (value > 100) {
    return 100;
  }
  return static_cast<uint8_t>(value);
}

uint32_t clampPlaylistDwellMs(int ms) {
  if (ms < PLAYLIST_DWELL_MS_MIN) {
    return PLAYLIST_DWELL_MS_MIN;
  }
  if (ms > PLAYLIST_DWELL_MS_MAX) {
    return PLAYLIST_DWELL_MS_MAX;
  }
  return static_cast<uint32_t>(ms);
}

int8_t clampContentOffset(int px) {
  if (px < -52) {
    return -52;
  }
  if (px > 52) {
    return 52;
  }
  return static_cast<int8_t>(px);
}

void PresetStore::setDefaults(SignPreset &preset) const {
  preset.contentType = ContentType::Text;
  preset.effectId = 0;
  preset.textHeightPx = TEXT_HEIGHT_PX_DEFAULT;
  preset.rowCount = 1;
  strncpy(preset.message, "MatrixPortal", sizeof(preset.message) - 1);
  preset.message[sizeof(preset.message) - 1] = '\0';
  preset.label[0] = '\0';
  preset.scroll = true;
  preset.scrollDelayMs = 40;
  preset.colorR = 255;
  preset.colorG = 255;
  preset.colorB = 255;
  preset.effectParam = 50;
  preset.countdownEndUnix = 0;
  preset.countdownDurationSec = 0;
  preset.contentOffsetX = 0;
  preset.contentOffsetY = 0;
  preset.gifPath[0] = '\0';
}

String PresetStore::gifPathForSlot(int index) const {
  if (index < 0 || index >= PRESET_COUNT) {
    return String();
  }
  return String("/gif/") + index + ".gif";
}

bool PresetStore::gifExistsForSlot(int index) const {
  const String path = gifPathForSlot(index);
  if (path.isEmpty()) {
    return false;
  }
  return LittleFS.exists(path);
}

void PresetStore::migrateLegacySign() {
  Preferences legacy;
  if (!legacy.begin("sign", true)) {
    return;
  }

  if (!legacy.isKey("text")) {
    legacy.end();
    return;
  }

  SignPreset preset;
  setDefaults(preset);
  String text = legacy.getString("text", preset.message);
  text.toCharArray(preset.message, sizeof(preset.message));
  preset.scroll = legacy.getBool("scroll", preset.scroll);
  preset.scrollDelayMs = legacy.getUShort("scrollMs", preset.scrollDelayMs);
  globalBrightness_ = clampBrightness(legacy.getUChar("bright", kDefaultBrightness));
  preset.colorR = legacy.getUChar("colorR", preset.colorR);
  preset.colorG = legacy.getUChar("colorG", preset.colorG);
  preset.colorB = legacy.getUChar("colorB", preset.colorB);
  legacy.end();

  activeIndex_ = 0;
  if (!set(0, preset)) {
    return;
  }
  saveActiveIndex();
  saveGlobalBrightness();

  Preferences clearLegacy;
  if (clearLegacy.begin("sign", false)) {
    clearLegacy.clear();
    clearLegacy.end();
  }
}

void PresetStore::loadAll() {
  StoreLock lock;

  for (int i = 0; i < PRESET_COUNT; i++) {
    setDefaults(presets_[i]);
    const String defaultPath = gifPathForSlot(i);
    defaultPath.toCharArray(presets_[i].gifPath, sizeof(presets_[i].gifPath));
  }

  strlcpy(timezoneId_, timeSyncDefaultTimezoneId(), sizeof(timezoneId_));

  Preferences prefs;
  if (!prefs.begin(kNs, true)) {
    timeSyncApplyTimezone(timezoneId_);
    return;
  }

  activeIndex_ = prefs.getInt("active", 0);
  if (activeIndex_ < 0 || activeIndex_ >= PRESET_COUNT) {
    activeIndex_ = 0;
  }

  uint8_t migrateBrightness = kDefaultBrightness;
  bool hasGlobalBrightness = prefs.isKey("brightness");
  if (hasGlobalBrightness) {
    globalBrightness_ = clampBrightness(prefs.getUChar("brightness", kDefaultBrightness));
  }

  playlist_.enabled = prefs.getBool("plOn", false);
  playlist_.slotMask = prefs.getUChar("plMask", 0xFF);
  playlist_.dwellMs = clampPlaylistDwellMs(prefs.getUInt("plDwell", PLAYLIST_DWELL_MS_DEFAULT));
  displayOn_ = prefs.getBool("dispOn", true);
  if (prefs.isKey("tzId")) {
    String tz = prefs.getString("tzId", timezoneId_);
    tz.toCharArray(timezoneId_, sizeof(timezoneId_));
  }
  {
    const char *normalized = timeSyncNormalizeTimezoneId(timezoneId_);
    strlcpy(timezoneId_, normalized, sizeof(timezoneId_));
  }

  for (int i = 0; i < PRESET_COUNT; i++) {
    char key[4];
    snprintf(key, sizeof(key), "p%d", i);
    const String json = prefs.getString(key, "");
    if (json.isEmpty()) {
      continue;
    }

    JsonDocument doc;
    if (deserializeJson(doc, json) || doc.overflowed()) {
      Serial.printf("preset %s: load failed (%u bytes)\n", key, json.length());
      continue;
    }

    SignPreset preset;
    setDefaults(preset);
    copyJsonStringValue(doc["message"], preset.message, sizeof(preset.message));
    if (preset.message[0] == '\0') {
      copyJsonStringValue(doc["text"], preset.message, sizeof(preset.message));
    }
    copyJsonStringValue(doc["slotLabel"], preset.label, sizeof(preset.label));
    if (preset.label[0] == '\0') {
      copyJsonStringValue(doc["label"], preset.label, sizeof(preset.label));
    }
    preset.scroll = doc["scroll"] | preset.scroll;
    preset.scrollDelayMs = doc["scrollDelayMs"] | preset.scrollDelayMs;
    if (doc["brightness"].is<int>() && i == activeIndex_) {
      migrateBrightness = static_cast<uint8_t>(doc["brightness"].as<int>());
    }
    preset.colorR = doc["colorR"] | preset.colorR;
    preset.colorG = doc["colorG"] | preset.colorG;
    preset.colorB = doc["colorB"] | preset.colorB;
    preset.effectParam = doc["effectParam"] | preset.effectParam;
    preset.countdownEndUnix = doc["countdownEndUnix"] | preset.countdownEndUnix;
    preset.countdownDurationSec = doc["countdownDurationSec"] | preset.countdownDurationSec;
    if (doc["contentOffsetX"].is<int>()) {
      preset.contentOffsetX = clampContentOffset(doc["contentOffsetX"].as<int>());
    }
    if (doc["contentOffsetY"].is<int>()) {
      preset.contentOffsetY = clampContentOffset(doc["contentOffsetY"].as<int>());
    }
    if (doc["gifPath"].is<const char *>()) {
      strlcpy(preset.gifPath, doc["gifPath"], sizeof(preset.gifPath));
    }
    if (doc["contentType"].is<const char *>()) {
      preset.contentType = contentTypeFromString(doc["contentType"]);
    }
    if (doc["effectId"].is<const char *>()) {
      preset.effectId = static_cast<uint8_t>(effectIdFromString(doc["effectId"]));
    } else if (doc["effectId"].is<int>()) {
      preset.effectId = static_cast<uint8_t>(doc["effectId"].as<int>());
    }
    if (doc["textHeightPx"].is<int>()) {
      preset.textHeightPx = clampTextHeightPx(doc["textHeightPx"].as<int>());
    } else if (doc["fontScale"].is<const char *>()) {
      preset.textHeightPx = textHeightPxFromLegacyFontScale(doc["fontScale"]);
    }
    preset.rowCount = doc["rowCount"] | preset.rowCount;
    if (preset.rowCount < 1) {
      preset.rowCount = 1;
    }
    if (preset.rowCount > 4) {
      preset.rowCount = 4;
    }
    presets_[i] = preset;
  }

  if (!hasGlobalBrightness) {
    globalBrightness_ = clampBrightness(migrateBrightness);
    saveGlobalBrightness();
  }

  timeSyncApplyTimezone(timezoneId_);
  prefs.end();
}

void PresetStore::saveGlobalBrightness() {
  StoreLock lock;
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putUChar("brightness", globalBrightness_);
  prefs.end();
}

bool PresetStore::persistPreset(int index, const SignPreset &preset) {
  if (index < 0 || index >= PRESET_COUNT) {
    return false;
  }

  char json[kPresetJsonMax];
  if (!buildPresetJson(preset, json, sizeof(json))) {
    Serial.printf("preset p%d: JSON build failed\n", index);
    return false;
  }

  char key[4];
  snprintf(key, sizeof(key), "p%d", index);

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    Serial.printf("preset p%d: NVS open failed\n", index);
    return false;
  }
  const size_t written = prefs.putString(key, json);
  prefs.end();
  if (written == 0) {
    Serial.printf("preset p%d: NVS write failed\n", index);
    return false;
  }
  return true;
}

void PresetStore::saveActiveIndex() {
  StoreLock lock;
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putInt("active", activeIndex_);
  prefs.end();
}

void PresetStore::savePlaylist() {
  StoreLock lock;
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putBool("plOn", playlist_.enabled);
  prefs.putUChar("plMask", playlist_.slotMask);
  prefs.putUInt("plDwell", playlist_.dwellMs);
  prefs.end();
}

void PresetStore::saveTimezoneId() {
  StoreLock lock;
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putString("tzId", timezoneId_);
  prefs.end();
}

void PresetStore::saveDisplayOn() {
  StoreLock lock;
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putBool("dispOn", displayOn_);
  prefs.end();
}

bool PresetStore::duplicateSlot(int fromIndex, int toIndex) {
  StoreLock lock;
  if (fromIndex < 0 || fromIndex >= PRESET_COUNT || toIndex < 0 || toIndex >= PRESET_COUNT ||
      fromIndex == toIndex) {
    return false;
  }

  presets_[toIndex] = presets_[fromIndex];
  const String toPath = gifPathForSlot(toIndex);
  toPath.toCharArray(presets_[toIndex].gifPath, sizeof(presets_[toIndex].gifPath));

  const String fromPath = gifPathForSlot(fromIndex);
  if (LittleFS.exists(fromPath)) {
    if (LittleFS.exists(toPath)) {
      LittleFS.remove(toPath);
    }
    File in = LittleFS.open(fromPath, "r");
    if (!in) {
      return false;
    }
    File out = LittleFS.open(toPath, "w");
    if (!out) {
      in.close();
      return false;
    }
    uint8_t buffer[512];
    while (in.available()) {
      const size_t n = in.read(buffer, sizeof(buffer));
      if (n == 0) {
        break;
      }
      out.write(buffer, n);
    }
    in.close();
    out.close();
  } else if (LittleFS.exists(toPath)) {
    LittleFS.remove(toPath);
  }

  if (!persistPreset(toIndex, presets_[toIndex])) {
    return false;
  }
  return true;
}

PlaylistSettings PresetStore::playlist() const {
  StoreLock lock;
  return playlist_;
}

void PresetStore::setPlaylist(const PlaylistSettings &settings, bool persist) {
  StoreLock lock;
  playlist_.enabled = settings.enabled;
  playlist_.slotMask = settings.slotMask;
  playlist_.dwellMs = clampPlaylistDwellMs(settings.dwellMs);
  if (persist) {
    savePlaylist();
  }
}

const char *PresetStore::timezoneId() const {
  StoreLock lock;
  return timezoneId_;
}

void PresetStore::setTimezoneId(const char *id, bool persist) {
  StoreLock lock;
  strlcpy(timezoneId_, timeSyncNormalizeTimezoneId(id), sizeof(timezoneId_));
  timeSyncApplyTimezone(timezoneId_);
  if (persist) {
    saveTimezoneId();
  }
}

bool PresetStore::displayOn() const {
  StoreLock lock;
  return displayOn_;
}

void PresetStore::setDisplayOn(bool on, bool persist) {
  StoreLock lock;
  displayOn_ = on;
  if (persist) {
    saveDisplayOn();
  }
}

void PresetStore::begin() {
  if (storeMutex == nullptr) {
    storeMutex = xSemaphoreCreateRecursiveMutex();
  }
  StoreLock lock;
  loadAll();
  migrateLegacySign();
}

SignPreset PresetStore::get(int index) const {
  StoreLock lock;
  if (index < 0 || index >= PRESET_COUNT) {
    SignPreset empty{};
    return empty;
  }
  return presets_[index];
}

bool PresetStore::set(int index, const SignPreset &preset) {
  StoreLock lock;
  if (index < 0 || index >= PRESET_COUNT) {
    return false;
  }
  if (!persistPreset(index, preset)) {
    return false;
  }
  presets_[index] = preset;
  return true;
}

int PresetStore::activeIndex() const {
  StoreLock lock;
  return activeIndex_;
}

void PresetStore::setActiveIndex(int index, bool persist) {
  StoreLock lock;
  if (index < 0 || index >= PRESET_COUNT) {
    return;
  }
  activeIndex_ = index;
  if (persist) {
    saveActiveIndex();
  }
}

uint8_t PresetStore::globalBrightness() const {
  StoreLock lock;
  return globalBrightness_;
}

void PresetStore::setGlobalBrightness(uint8_t value, bool persist) {
  StoreLock lock;
  globalBrightness_ = clampBrightness(value);
  if (persist) {
    saveGlobalBrightness();
  }
}
