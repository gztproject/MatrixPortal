#include "preset_store.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <cstring>

namespace {
constexpr char kNs[] = "presets";
constexpr uint8_t kDefaultBrightness = 10;
}  // namespace

void PresetStore::setDefaults(SignPreset &preset) const {
  strncpy(preset.text, "MatrixPortal", sizeof(preset.text) - 1);
  preset.text[sizeof(preset.text) - 1] = '\0';
  preset.scroll = true;
  preset.scrollDelayMs = 40;
  preset.brightness = kDefaultBrightness;
  preset.colorR = 255;
  preset.colorG = 255;
  preset.colorB = 255;
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
  preset.brightness = legacy.getUChar("bright", preset.brightness);
  preset.colorR = legacy.getUChar("colorR", preset.colorR);
  preset.colorG = legacy.getUChar("colorG", preset.colorG);
  preset.colorB = legacy.getUChar("colorB", preset.colorB);
  legacy.end();

  presets_[0] = preset;
  activeIndex_ = 0;
  savePreset(0);
  saveActiveIndex();

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

  Preferences prefs;
  if (!prefs.begin(kNs, true)) {
    return;
  }

  activeIndex_ = prefs.getInt("active", 0);
  if (activeIndex_ < 0 || activeIndex_ >= PRESET_COUNT) {
    activeIndex_ = 0;
  }

  for (int i = 0; i < PRESET_COUNT; i++) {
    const String key = String("p") + i;
    const String json = prefs.getString(key.c_str(), "");
    if (json.isEmpty()) {
      continue;
    }

    JsonDocument doc;
    if (deserializeJson(doc, json)) {
      continue;
    }

    SignPreset preset;
    setDefaults(preset);
    if (doc["text"].is<const char *>()) {
      strlcpy(preset.text, doc["text"], sizeof(preset.text));
    }
    preset.scroll = doc["scroll"] | preset.scroll;
    preset.scrollDelayMs = doc["scrollDelayMs"] | preset.scrollDelayMs;
    preset.brightness = doc["brightness"] | preset.brightness;
    preset.colorR = doc["colorR"] | preset.colorR;
    preset.colorG = doc["colorG"] | preset.colorG;
    preset.colorB = doc["colorB"] | preset.colorB;
    if (doc["gifPath"].is<const char *>()) {
      strlcpy(preset.gifPath, doc["gifPath"], sizeof(preset.gifPath));
    }
    presets_[i] = preset;
  }

  prefs.end();
}

void PresetStore::savePreset(int index) {
  if (index < 0 || index >= PRESET_COUNT) {
    return;
  }

  JsonDocument doc;
  doc["text"] = presets_[index].text;
  doc["scroll"] = presets_[index].scroll;
  doc["scrollDelayMs"] = presets_[index].scrollDelayMs;
  doc["brightness"] = presets_[index].brightness;
  doc["colorR"] = presets_[index].colorR;
  doc["colorG"] = presets_[index].colorG;
  doc["colorB"] = presets_[index].colorB;
  doc["gifPath"] = presets_[index].gifPath;

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
