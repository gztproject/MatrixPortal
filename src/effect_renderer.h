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
};

struct EffectInfo {
  const char *id;
  const char *label;
  EffectId effect;
};

void effectRendererBegin(VirtualMatrixPanel *panel);
void effectRendererApply(EffectId id);
bool effectRendererTick(EffectId id);
const EffectInfo *effectCatalog(size_t *count);
EffectId effectIdFromString(const char *id);
const char *effectIdToString(EffectId id);
const char *effectLabel(EffectId id);
