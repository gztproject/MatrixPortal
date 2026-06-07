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
  void previewOnPanel(const SignPreset &preset, int gifSlotIndex);
  void setGlobalBrightness(uint8_t percent);
  void adjustGlobalBrightness(int delta);
  uint8_t globalBrightness() const;
  void refreshTimeDisplay();
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
  void applyRuntimePreset(int gifSlotIndex);
  void resolveActiveContentType();
  void applyBrightness(uint8_t brightnessPercent);
  TextLayout computeTextLayout(const SignPreset &preset, char lines[][201], int lineCount) const;
  int splitTextLines(const SignPreset &preset, char lines[][201], int maxLines) const;
  int textBlockPixelWidth(const SignPreset &preset, const TextLayout &layout,
                          char lines[][201], int lineCount) const;
  uint16_t textColor565(const SignPreset &preset) const;
  void redrawTextBlock();
  void redrawTimeBlock();
  void redrawOtaScreen(uint8_t progressPercent);
  void tickText(const SignPreset &preset);
  void tickTime(const SignPreset &preset);
  void tickGif(const SignPreset &preset);
  void tickEffect(const SignPreset &preset);
  void tickPlaylist();
  bool shouldPlayGif(const SignPreset &preset) const;
  int nextPlaylistSlot(int current) const;

  VirtualMatrixPanel *panel_ = nullptr;
  MatrixPanel_I2S_DMA *dma_ = nullptr;
  PresetStore *store_ = nullptr;
  SignPreset runtime_{};
  ContentType activeContentType_ = ContentType::Text;
  TextLayout textLayout_{};
  char textLines_[4][201]{};
  char timeLine_[32]{};
  char dateLine_[16]{};
  int textLineCount_ = 0;
  int scrollOffset_ = 0;
  unsigned long lastScrollMs_ = 0;
  unsigned long countdownStartedMs_ = 0;
  unsigned long playlistSlotSinceMs_ = 0;
  bool dirty_ = true;
};
