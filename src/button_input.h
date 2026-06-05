#pragma once

#include <Arduino.h>

class DisplayEngine;

typedef void (*PresetChangeCallback)(int newIndex, void *context);

void buttonInputBegin(DisplayEngine *engine, PresetChangeCallback callback = nullptr,
                      void *callbackContext = nullptr);
void buttonInputTick();
