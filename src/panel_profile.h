#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 104
#define PANEL_RES_Y 52

MatrixPanel_I2S_DMA *panelProfileDma();
VirtualMatrixPanel *panelProfileVirtual();
bool panelProfileBegin();
