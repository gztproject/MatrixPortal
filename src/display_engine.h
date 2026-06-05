#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

struct SignConfig {
  char text[201];
  bool scroll;
  uint16_t scrollDelayMs;
  uint8_t brightness;
  uint8_t colorR;
  uint8_t colorG;
  uint8_t colorB;
};

class DisplayEngine {
 public:
  void begin(VirtualMatrixPanel *panel, MatrixPanel_I2S_DMA *dma);
  void loadFromNvs();
  void saveToNvs();
  SignConfig getConfig() const;
  void applyConfig(const SignConfig &cfg, bool persist = true);
  void tick();

 private:
  void redrawTextBand();
  int textPixelWidth() const;
  uint16_t textColor565() const;
  void applyBrightness();

  VirtualMatrixPanel *panel_ = nullptr;
  MatrixPanel_I2S_DMA *dma_ = nullptr;
  SignConfig config_{};
  int scrollOffset_ = 0;
  unsigned long lastScrollMs_ = 0;
  bool dirty_ = true;
};
