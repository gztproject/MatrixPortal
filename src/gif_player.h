#pragma once

#include <Arduino.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

bool gifPlayerBegin(VirtualMatrixPanel *panel);
void gifPlayerEnd();
bool gifPlayerOpen(const char *path);
void gifPlayerClose();
bool gifPlayerIsOpen();
bool gifPlayerTick();
bool gifPlayerFileExists(const char *path);
