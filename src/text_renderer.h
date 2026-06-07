#pragma once

#include <Adafruit_GFX.h>

#include <cstdint>

// Space above the 8-row GFX glyph: 2-row caron + 1-row gap (in textSize units).
constexpr int kTextCaronBandRows = 3;
constexpr int kTextBodyBandRows = 8;

int textLinePixelWidth(const char *text, int textSize);
bool textLineHasCaron(const char *text);
void drawTextLine(Adafruit_GFX *gfx, int x, int y, int textSize, uint16_t color, const char *text);
