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
  void setGlobalBrightness(uint8_t percent);
  uint8_t globalBrightness() const;
  void tick();

 private:
  struct TextLayout {
    int blockTop = 0;
    int blockHeight = 0;
    int textSize = 1;
    int rowCount = 1;
    int rowY[4]{};
  };

  void applyActivePreset();
  void resolveActiveContentType();
  void applyBrightness(uint8_t brightnessPercent);
  TextLayout computeTextLayout(const SignPreset &preset) const;
  int splitTextLines(const SignPreset &preset, char lines[][201], int maxLines) const;
  int textBlockPixelWidth(const SignPreset &preset, const TextLayout &layout,
                          char lines[][201], int lineCount) const;
  uint16_t textColor565(const SignPreset &preset) const;
  void redrawTextBlock();
  void showPresetFlash(int index);
  void tickText(const SignPreset &preset);
  void tickGif(const SignPreset &preset);
  void tickEffect(const SignPreset &preset);
  bool shouldPlayGif(const SignPreset &preset) const;

  VirtualMatrixPanel *panel_ = nullptr;
  MatrixPanel_I2S_DMA *dma_ = nullptr;
  PresetStore *store_ = nullptr;
  SignPreset runtime_{};
  ContentType activeContentType_ = ContentType::Text;
  TextLayout textLayout_{};
  char textLines_[4][201]{};
  int textLineCount_ = 0;
  int scrollOffset_ = 0;
  unsigned long lastScrollMs_ = 0;
  bool dirty_ = true;
  unsigned long flashUntilMs_ = 0;
};
