#pragma once

#include <Arduino.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

enum class EffectId : uint8_t {
  None = 0,
  BrightWhite,
  BlueEmergency,
  YellowEmergency,
  ArrowLeft,
  ArrowRight,
  Stop,
  HazardTriangle,
  FullStrobe,
  Pulse,
  BorderChase,
  ProgressBar,
  GameOfLife,
};

struct EffectInfo {
  const char *id;
  const char *label;
  EffectId effect;
  bool monochrome;
};

void effectRendererBegin(VirtualMatrixPanel *panel);
void effectRendererApply(EffectId id, uint8_t r, uint8_t g, uint8_t b, uint8_t param = 50);
bool effectRendererTick(EffectId id);
const EffectInfo *effectCatalog(size_t *count);
bool effectIsMonochrome(EffectId id);
EffectId effectIdFromString(const char *id);
const char *effectIdToString(EffectId id);
const char *effectLabel(EffectId id);
