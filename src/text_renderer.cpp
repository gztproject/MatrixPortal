#include "text_renderer.h"

namespace {

enum AccentKind : uint8_t { kAccentNone = 0, kAccentCaron = 1 };

constexpr int kGlyphAdvanceCols = 6;

bool utf8NextCodepoint(const char *&cursor, uint32_t &codepoint) {
  const unsigned char lead = static_cast<unsigned char>(*cursor);
  if (lead == 0) {
    return false;
  }

  if (lead < 0x80) {
    codepoint = lead;
    cursor++;
    return true;
  }

  int extraBytes = 0;
  uint32_t value = 0;
  if ((lead & 0xE0) == 0xC0) {
    extraBytes = 1;
    value = lead & 0x1F;
  } else if ((lead & 0xF0) == 0xE0) {
    extraBytes = 2;
    value = lead & 0x0F;
  } else if ((lead & 0xF8) == 0xF0) {
    extraBytes = 3;
    value = lead & 0x07;
  } else {
    cursor++;
    codepoint = '?';
    return true;
  }

  for (int i = 0; i < extraBytes; i++) {
    const unsigned char follow = static_cast<unsigned char>(cursor[i + 1]);
    if ((follow & 0xC0) != 0x80) {
      cursor++;
      codepoint = '?';
      return true;
    }
    value = (value << 6) | (follow & 0x3F);
  }

  cursor += extraBytes + 1;
  codepoint = value;
  return true;
}

bool mapExtendedGlyph(uint32_t codepoint, char &base, AccentKind &accent) {
  switch (codepoint) {
    case 0x010D:  // č
      base = 'c';
      accent = kAccentCaron;
      return true;
    case 0x010C:  // Č
      base = 'C';
      accent = kAccentCaron;
      return true;
    case 0x0161:  // š
      base = 's';
      accent = kAccentCaron;
      return true;
    case 0x0160:  // Š
      base = 'S';
      accent = kAccentCaron;
      return true;
    case 0x017E:  // ž
      base = 'z';
      accent = kAccentCaron;
      return true;
    case 0x017D:  // Ž
      base = 'Z';
      accent = kAccentCaron;
      return true;
    default:
      return false;
  }
}

void drawCaron(Adafruit_GFX *gfx, int x, int y, int textSize, uint16_t color) {
  // Caron (ˇ): top row wings at cols 0 and 2, center point at col 1 below.
  static const uint8_t kCaronRows[] = {0b10100, 0b01000};

  for (int row = 0; row < 2; row++) {
    const uint8_t bits = kCaronRows[row];
    for (int col = 0; col < 5; col++) {
      if (bits & (1 << (4 - col))) {
        gfx->fillRect(x + col * textSize, y + row * textSize, textSize, textSize, color);
      }
    }
  }
}

void drawGlyph(Adafruit_GFX *gfx, int x, int y, int textSize, uint16_t color, char base,
               AccentKind accent) {
  gfx->drawChar(x, y, static_cast<unsigned char>(base), color, 0, textSize);
  if (accent == kAccentCaron) {
    drawCaron(gfx, x, y - kTextCaronBandRows * textSize, textSize, color);
  }
}

int countGlyphs(const char *text) {
  int count = 0;
  const char *cursor = text;
  uint32_t codepoint = 0;
  while (utf8NextCodepoint(cursor, codepoint)) {
    count++;
  }
  return count;
}

bool lineHasCaronAccent(const char *text) {
  const char *cursor = text;
  uint32_t codepoint = 0;
  while (utf8NextCodepoint(cursor, codepoint)) {
    char base = '?';
    AccentKind accent = kAccentNone;
    if (mapExtendedGlyph(codepoint, base, accent) && accent == kAccentCaron) {
      return true;
    }
  }
  return false;
}

}  // namespace

int textLinePixelWidth(const char *text, int textSize) {
  return countGlyphs(text) * kGlyphAdvanceCols * textSize;
}

bool textLineHasCaron(const char *text) {
  return lineHasCaronAccent(text);
}

void drawTextLine(Adafruit_GFX *gfx, int x, int y, int textSize, uint16_t color,
                  const char *text) {
  int cursorX = x;
  const char *cursor = text;
  uint32_t codepoint = 0;

  while (utf8NextCodepoint(cursor, codepoint)) {
    if (codepoint < 0x80) {
      drawGlyph(gfx, cursorX, y, textSize, color, static_cast<char>(codepoint), kAccentNone);
    } else {
      char base = '?';
      AccentKind accent = kAccentNone;
      if (mapExtendedGlyph(codepoint, base, accent)) {
        drawGlyph(gfx, cursorX, y, textSize, color, base, accent);
      } else {
        drawGlyph(gfx, cursorX, y, textSize, color, '?', kAccentNone);
      }
    }
    cursorX += kGlyphAdvanceCols * textSize;
  }
}
