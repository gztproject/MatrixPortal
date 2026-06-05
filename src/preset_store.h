#pragma once

#include <Arduino.h>

#define PRESET_COUNT 8
#define MAX_GIF_BYTES (256 * 1024)

struct SignPreset {
  char text[201];
  bool scroll;
  uint16_t scrollDelayMs;
  uint8_t brightness;
  uint8_t colorR;
  uint8_t colorG;
  uint8_t colorB;
  char gifPath[48];
};

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
