#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

#include "preset_store.h"

class DisplayEngine {
 public:
  void begin(VirtualMatrixPanel *panel, MatrixPanel_I2S_DMA *dma, PresetStore *store);
  PresetStore *store() { return store_; }

  int activeIndex() const;
  SignPreset activePreset() const;
  void selectPreset(int index);
  void applyPreset(const SignPreset &preset, int index, bool persist = true);
  void tick();

 private:
  void applyActivePreset();
  void applyBrightness(uint8_t brightnessPercent);
  void redrawTextBand();
  int textPixelWidth(const SignPreset &preset) const;
  uint16_t textColor565(const SignPreset &preset) const;
  void showPresetFlash(int index);
  void tickText(const SignPreset &preset);
  void tickGif(const SignPreset &preset);
  bool shouldPlayGif(const SignPreset &preset) const;

  VirtualMatrixPanel *panel_ = nullptr;
  MatrixPanel_I2S_DMA *dma_ = nullptr;
  PresetStore *store_ = nullptr;
  SignPreset runtime_{};
  int scrollOffset_ = 0;
  unsigned long lastScrollMs_ = 0;
  bool dirty_ = true;
  bool gifMode_ = false;
  unsigned long flashUntilMs_ = 0;
  int flashIndex_ = -1;
};
