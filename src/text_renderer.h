#pragma once

#include <Adafruit_GFX.h>

#include <cstdint>

// Vertical band reserved above the standard 8-row GFX glyph for carons (in textSize units).
constexpr int kTextCaronBandRows = 2;
constexpr int kTextBodyBandRows = 8;

int textLinePixelWidth(const char *text, int textSize);
bool textLineHasCaron(const char *text);
void drawTextLine(Adafruit_GFX *gfx, int x, int y, int textSize, uint16_t color, const char *text);
