#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 104
#define PANEL_RES_Y 52

MatrixPanel_I2S_DMA *panelProfileDma();
VirtualMatrixPanel *panelProfileVirtual();
bool panelProfileBegin();

// Compensate for 13-row tile X remapping so multi-row / scrolling text stays aligned.
int16_t panelAlignVirtualX(int16_t virtX, int16_t virtY, int16_t refVirtY);
