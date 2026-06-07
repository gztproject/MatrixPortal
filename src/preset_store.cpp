#include "preset_store.h"

#include "effect_renderer.h"
#include "panel_profile.h"
#include "time_sync.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <cstring>

namespace {
constexpr char kNs[] = "presets";
constexpr uint8_t kDefaultBrightness = 10;

void assignJsonString(JsonVariantConst value, char *dest, size_t destSize) {
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
  strncpy(preset.text, "MatrixPortal", sizeof(preset.text) - 1);
  preset.text[sizeof(preset.text) - 1] = '\0';
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
  String text = legacy.getString("text", preset.text);
  text.toCharArray(preset.text, sizeof(preset.text));
  preset.scroll = legacy.getBool("scroll", preset.scroll);
  preset.scrollDelayMs = legacy.getUShort("scrollMs", preset.scrollDelayMs);
  globalBrightness_ = clampBrightness(legacy.getUChar("bright", kDefaultBrightness));
  preset.colorR = legacy.getUChar("colorR", preset.colorR);
  preset.colorG = legacy.getUChar("colorG", preset.colorG);
  preset.colorB = legacy.getUChar("colorB", preset.colorB);
  legacy.end();

  presets_[0] = preset;
  activeIndex_ = 0;
  savePreset(0);
  saveActiveIndex();
  saveGlobalBrightness();

  Preferences clearLegacy;
  if (clearLegacy.begin("sign", false)) {
    clearLegacy.clear();
    clearLegacy.end();
  }
}

void PresetStore::loadAll() {
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
  if (prefs.isKey("tzId")) {
    String tz = prefs.getString("tzId", timezoneId_);
    tz.toCharArray(timezoneId_, sizeof(timezoneId_));
  }
  {
    const char *normalized = timeSyncNormalizeTimezoneId(timezoneId_);
    strlcpy(timezoneId_, normalized, sizeof(timezoneId_));
  }

  for (int i = 0; i < PRESET_COUNT; i++) {
    const String key = String("p") + i;
    const String json = prefs.getString(key.c_str(), "");
    if (json.isEmpty()) {
      continue;
    }

    JsonDocument doc;
    if (deserializeJson(doc, json) || doc.overflowed()) {
      continue;
    }

    SignPreset preset;
    setDefaults(preset);
    assignJsonString(doc["text"], preset.text, sizeof(preset.text));
    assignJsonString(doc["label"], preset.label, sizeof(preset.label));
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
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putUChar("brightness", globalBrightness_);
  prefs.end();
}

void PresetStore::savePreset(int index) {
  if (index < 0 || index >= PRESET_COUNT) {
    return;
  }

  JsonDocument doc;
  doc["contentType"] = contentTypeToString(presets_[index].contentType);
  doc["effectId"] = effectIdToString(static_cast<EffectId>(presets_[index].effectId));
  doc["textHeightPx"] = presets_[index].textHeightPx;
  doc["rowCount"] = presets_[index].rowCount;
  doc["text"] = presets_[index].text;
  doc["label"] = presets_[index].label;
  doc["scroll"] = presets_[index].scroll;
  doc["scrollDelayMs"] = presets_[index].scrollDelayMs;
  doc["colorR"] = presets_[index].colorR;
  doc["colorG"] = presets_[index].colorG;
  doc["colorB"] = presets_[index].colorB;
  doc["effectParam"] = presets_[index].effectParam;
  doc["countdownEndUnix"] = presets_[index].countdownEndUnix;
  doc["countdownDurationSec"] = presets_[index].countdownDurationSec;
  doc["contentOffsetX"] = presets_[index].contentOffsetX;
  doc["contentOffsetY"] = presets_[index].contentOffsetY;
  doc["gifPath"] = presets_[index].gifPath;

  if (doc.overflowed()) {
    return;
  }

  String json;
  serializeJson(doc, json);

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  const String key = String("p") + index;
  prefs.putString(key.c_str(), json);
  prefs.end();
}

void PresetStore::saveActiveIndex() {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putInt("active", activeIndex_);
  prefs.end();
}

void PresetStore::savePlaylist() {
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
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putString("tzId", timezoneId_);
  prefs.end();
}

bool PresetStore::duplicateSlot(int fromIndex, int toIndex) {
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

  savePreset(toIndex);
  return true;
}

PlaylistSettings PresetStore::playlist() const {
  return playlist_;
}

void PresetStore::setPlaylist(const PlaylistSettings &settings, bool persist) {
  playlist_.enabled = settings.enabled;
  playlist_.slotMask = settings.slotMask;
  playlist_.dwellMs = clampPlaylistDwellMs(settings.dwellMs);
  if (persist) {
    savePlaylist();
  }
}

const char *PresetStore::timezoneId() const {
  return timezoneId_;
}

void PresetStore::setTimezoneId(const char *id, bool persist) {
  strlcpy(timezoneId_, timeSyncNormalizeTimezoneId(id), sizeof(timezoneId_));
  timeSyncApplyTimezone(timezoneId_);
  if (persist) {
    saveTimezoneId();
  }
}

void PresetStore::begin() {
  loadAll();
  migrateLegacySign();
}

SignPreset PresetStore::get(int index) const {
  if (index < 0 || index >= PRESET_COUNT) {
    SignPreset empty{};
    return empty;
  }
  return presets_[index];
}

void PresetStore::set(int index, const SignPreset &preset) {
  if (index < 0 || index >= PRESET_COUNT) {
    return;
  }
  presets_[index] = preset;
  savePreset(index);
}

int PresetStore::activeIndex() const {
  return activeIndex_;
}

void PresetStore::setActiveIndex(int index, bool persist) {
  if (index < 0 || index >= PRESET_COUNT) {
    return;
  }
  activeIndex_ = index;
  if (persist) {
    saveActiveIndex();
  }
}

uint8_t PresetStore::globalBrightness() const {
  return globalBrightness_;
}

void PresetStore::setGlobalBrightness(uint8_t value, bool persist) {
  globalBrightness_ = clampBrightness(value);
  if (persist) {
    saveGlobalBrightness();
  }
}
