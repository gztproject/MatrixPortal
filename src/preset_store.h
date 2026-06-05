#pragma once

#include <Arduino.h>

#define PRESET_COUNT 8
#define MAX_GIF_BYTES (256 * 1024)

enum class ContentType : uint8_t { Text, Gif, Effect };

enum class FontScale : uint8_t { Quarter, Half, ThreeQuarter, Full };

struct SignPreset {
  ContentType contentType;
  uint8_t effectId;
  FontScale fontScale;
  uint8_t rowCount;
  char text[201];
  bool scroll;
  uint16_t scrollDelayMs;
  uint8_t brightness;
  uint8_t colorR;
  uint8_t colorG;
  uint8_t colorB;
  char gifPath[48];
};

const char *contentTypeToString(ContentType type);
ContentType contentTypeFromString(const char *value);
const char *fontScaleToString(FontScale scale);
FontScale fontScaleFromString(const char *value);

class PresetStore {
 public:
  void begin();
  SignPreset get(int index) const;
  void set(int index, const SignPreset &preset);
  int activeIndex() const;
  void setActiveIndex(int index, bool persist = true);
  String gifPathForSlot(int index) const;
  bool gifExistsForSlot(int index) const;

 private:
  void loadAll();
  void savePreset(int index);
  void saveActiveIndex();
  void migrateLegacySign();
  void setDefaults(SignPreset &preset) const;

  SignPreset presets_[PRESET_COUNT]{};
  int activeIndex_ = 0;
};
