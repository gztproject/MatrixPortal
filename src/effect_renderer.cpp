#include "effect_renderer.h"

#include <cstring>

#include "panel_profile.h"

namespace {
VirtualMatrixPanel *panel = nullptr;
EffectId activeEffect = EffectId::None;
uint8_t effectColorR = 255;
uint8_t effectColorG = 255;
uint8_t effectColorB = 255;
uint8_t effectParam = 50;
unsigned long lastPhaseMs = 0;
uint8_t arrowPhase = 0;
unsigned long lastArrowMs = 0;

constexpr unsigned long kArrowMs = 120;
constexpr int kChevronWidth = PANEL_RES_Y / 2;
constexpr int kChevronBarWidth = 10;
constexpr int kChevronBarHeight = 1;
constexpr int kChevronBars = PANEL_RES_Y;
constexpr int kArrowSteps = 8;
constexpr int kArrowStepX = (PANEL_RES_X - kChevronWidth) / (kArrowSteps - 1);

struct EmergencyHalfParams {
  uint8_t flashCount;
  unsigned long flashOnMs;
  unsigned long flashOffMs;
  unsigned long holdOnMs;
};

enum class EmergencyStep : uint8_t { FlashOn, FlashOff, HoldOn };

constexpr EmergencyHalfParams kDefaultEmergencyHalf = {2, 120, 120, 0};

struct EmergencyTiming {
  EmergencyHalfParams left;
  EmergencyHalfParams right;
};

uint8_t emergencySide = 0;
uint8_t emergencyFlashDone = 0;
bool emergencyLit = false;
EmergencyStep emergencyStep = EmergencyStep::FlashOn;
EmergencyTiming emergencyTiming = {kDefaultEmergencyHalf, kDefaultEmergencyHalf};
bool emergencyActive = false;

constexpr int kLifeCols = 52;
constexpr int kLifeRows = 26;
uint8_t lifeGrid[kLifeCols * kLifeRows]{};
uint8_t lifeNext[kLifeCols * kLifeRows]{};
unsigned long lastLifeMs = 0;
bool strobeLit = true;
unsigned long lastStrobeMs = 0;
uint8_t pulsePhase = 0;
unsigned long lastPulseMs = 0;
uint16_t borderPos = 0;
unsigned long lastBorderMs = 0;

static const EffectInfo kCatalog[] = {
    {"bright_white", "Bright fill", EffectId::BrightWhite, true},
    {"flashing_halves", "Flashing halves", EffectId::BlueEmergency, true},
    {"full_strobe", "Full strobe", EffectId::FullStrobe, true},
    {"pulse", "Pulse", EffectId::Pulse, true},
    {"border_chase", "Border chase", EffectId::BorderChase, true},
    {"progress_bar", "Progress bar", EffectId::ProgressBar, true},
    {"game_of_life", "Game of Life", EffectId::GameOfLife, true},
    {"arrow_left", "Arrows left", EffectId::ArrowLeft, true},
    {"arrow_right", "Arrows right", EffectId::ArrowRight, true},
    {"stop", "Stop", EffectId::Stop, false},
    {"hazard_triangle", "Hazard triangle", EffectId::HazardTriangle, false},
};

uint16_t effectColor565() {
  return panel->color565(effectColorR, effectColorG, effectColorB);
}

void drawEmergency(uint8_t side, bool lit) {
  panel->fillScreen(0);
  if (!lit) {
    return;
  }

  const uint16_t color = effectColor565();
  const int halfWidth = PANEL_RES_X / 2;
  if (side == 0) {
    panel->fillRect(0, 0, halfWidth, PANEL_RES_Y, color);
  } else {
    panel->fillRect(halfWidth, 0, PANEL_RES_X - halfWidth, PANEL_RES_Y, color);
  }
}

void resetEmergencyState() {
  emergencyTiming.left = kDefaultEmergencyHalf;
  emergencyTiming.right = kDefaultEmergencyHalf;
  emergencyActive = true;
  emergencySide = 0;
  emergencyFlashDone = 0;
  emergencyLit = true;
  emergencyStep = EmergencyStep::FlashOn;
  lastPhaseMs = millis();
  drawEmergency(emergencySide, emergencyLit);
}

void advanceEmergencySide() {
  emergencySide ^= 1;
  emergencyFlashDone = 0;
  emergencyLit = true;
  emergencyStep = EmergencyStep::FlashOn;
}

bool tickEmergency() {
  if (!emergencyActive) {
    return false;
  }

  const EmergencyHalfParams &half =
      (emergencySide == 0) ? emergencyTiming.left : emergencyTiming.right;

  unsigned long duration = 0;
  switch (emergencyStep) {
    case EmergencyStep::FlashOn:
      duration = half.flashOnMs;
      break;
    case EmergencyStep::FlashOff:
      duration = half.flashOffMs;
      break;
    case EmergencyStep::HoldOn:
      duration = half.holdOnMs;
      break;
  }

  const unsigned long now = millis();
  if (now - lastPhaseMs < duration) {
    return false;
  }

  lastPhaseMs = now;

  switch (emergencyStep) {
    case EmergencyStep::FlashOn:
      emergencyLit = false;
      emergencyStep = EmergencyStep::FlashOff;
      break;

    case EmergencyStep::FlashOff:
      emergencyFlashDone++;
      if (emergencyFlashDone >= half.flashCount) {
        if (half.holdOnMs > 0) {
          emergencyLit = true;
          emergencyStep = EmergencyStep::HoldOn;
        } else {
          advanceEmergencySide();
        }
      } else {
        emergencyLit = true;
        emergencyStep = EmergencyStep::FlashOn;
      }
      break;

    case EmergencyStep::HoldOn:
      advanceEmergencySide();
      break;
  }

  drawEmergency(emergencySide, emergencyLit);
  return true;
}

void drawBoldChevron(bool right, int x, uint16_t color) {
  const int maxOffset = kChevronWidth - kChevronBarWidth;
  const int centerBar = (kChevronBars - 1) / 2;
  const int totalHeight = kChevronBars * kChevronBarHeight;
  const int startY = (PANEL_RES_Y - totalHeight) / 2;

  for (int i = 0; i < kChevronBars; i++) {
    const int y = startY + i * kChevronBarHeight;
    const int distFromCenter = abs(i - centerBar);
    const int offset = (centerBar == 0) ? 0 : (maxOffset * distFromCenter) / centerBar;
    const int barX = right ? (x + maxOffset - offset) : (x + offset);
    panel->fillRect(barX, y, kChevronBarWidth, kChevronBarHeight, color);
  }
}

void drawArrowSequence(bool right) {
  panel->fillScreen(0);
  const uint16_t color = effectColor565();

  const int phase = arrowPhase % kArrowSteps;
  const int x =
      right ? phase * kArrowStepX : (PANEL_RES_X - kChevronWidth - phase * kArrowStepX);
  drawBoldChevron(right, x, color);
}

void drawStop() {
  panel->fillScreen(panel->color565(200, 0, 0));
  constexpr int kTextSize = 4;
  constexpr const char kStopText[] = "STOP";
  const int textWidth = static_cast<int>(strlen(kStopText)) * 6 * kTextSize;
  const int textHeight = 8 * kTextSize;
  panel->setTextSize(kTextSize);
  panel->setTextWrap(false);
  panel->setTextColor(panel->color565(255, 255, 255));
  panel->setCursor((PANEL_RES_X - textWidth) / 2 + 2, (PANEL_RES_Y - textHeight) / 2);
  panel->print(kStopText);
}

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
  if (y0 > y1) {
    int t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }
  if (y1 > y2) {
    int t = x1;
    x1 = x2;
    x2 = t;
    t = y1;
    y1 = y2;
    y2 = t;
  }
  if (y0 > y1) {
    int t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }

  for (int y = y0; y <= y2; y++) {
    int xa = x0;
    int xb = x0;
    if (y <= y1) {
      if (y1 != y0) {
        xa = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
      }
      if (y2 != y0) {
        xb = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
      }
    } else {
      if (y2 != y1) {
        xa = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
      }
      xb = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    }
    if (xa > xb) {
      const int t = xa;
      xa = xb;
      xb = t;
    }
    panel->drawFastHLine(xa, y, xb - xa + 1, color);
  }
}

void drawHazardTriangle() {
  panel->fillScreen(0);

  // Equilateral triangle sized to fit panel height (side s, height h = s * sqrt(3) / 2).
  const int side = 50;
  const int height = (side * 866) / 1000;
  const int cx = PANEL_RES_X / 2;
  const int topY = (PANEL_RES_Y - height) / 2;
  const int baseY = topY + height;
  const int halfBase = side / 2;
  const int baseLeft = cx - halfBase;
  const int baseRight = cx + halfBase;

  const uint16_t amber = panel->color565(255, 170, 0);
  const uint16_t red = panel->color565(200, 0, 0);
  fillTriangle(cx, topY, baseLeft, baseY, baseRight, baseY, amber);

  panel->drawLine(cx, topY, baseLeft, baseY, red);
  panel->drawLine(cx, topY, baseRight, baseY, red);
  panel->drawLine(baseLeft, baseY, baseRight, baseY, red);

  panel->setTextSize(2);
  panel->setTextWrap(false);
  panel->setTextColor(panel->color565(0, 0, 0));
  const int markY = topY + (2 * height) / 3 - 8;
  panel->setCursor(cx - 5, markY);
  panel->print("!");
}

bool lifeAt(int x, int y) {
  if (x < 0 || y < 0 || x >= kLifeCols || y >= kLifeRows) {
    return false;
  }
  return lifeGrid[y * kLifeCols + x] != 0;
}

int countLifeNeighbors(int x, int y) {
  int n = 0;
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      if (lifeAt(x + dx, y + dy)) {
        n++;
      }
    }
  }
  return n;
}

void seedLife() {
  for (int i = 0; i < kLifeCols * kLifeRows; i++) {
    lifeGrid[i] = (esp_random() & 3) == 0 ? 1 : 0;
  }
}

void drawLife() {
  panel->fillScreen(0);
  const uint16_t color = effectColor565();
  for (int y = 0; y < kLifeRows; y++) {
    for (int x = 0; x < kLifeCols; x++) {
      if (!lifeGrid[y * kLifeCols + x]) {
        continue;
      }
      const int px = x * 2;
      const int py = y * 2;
      panel->fillRect(px, py, 2, 2, color);
    }
  }
}

void stepLife() {
  for (int y = 0; y < kLifeRows; y++) {
    for (int x = 0; x < kLifeCols; x++) {
      const int n = countLifeNeighbors(x, y);
      const bool alive = lifeAt(x, y);
      bool next = false;
      if (alive) {
        next = (n == 2 || n == 3);
      } else {
        next = (n == 3);
      }
      lifeNext[y * kLifeCols + x] = next ? 1 : 0;
    }
  }
  memcpy(lifeGrid, lifeNext, sizeof(lifeGrid));
}

void drawProgressBar() {
  panel->fillScreen(0);
  const uint16_t color = effectColor565();
  const int width = (PANEL_RES_X * effectParam) / 100;
  if (width > 0) {
    panel->fillRect(0, PANEL_RES_Y / 2 - 4, width, 8, color);
  }
}

void drawFullStrobe() {
  if (strobeLit) {
    panel->fillScreen(effectColor565());
  } else {
    panel->fillScreen(0);
  }
}

void drawPulse() {
  const uint8_t level = 64 + (pulsePhase % 192);
  const uint16_t color =
      panel->color565((effectColorR * level) / 255, (effectColorG * level) / 255,
                      (effectColorB * level) / 255);
  panel->fillScreen(color);
}

void drawBorderChase() {
  panel->fillScreen(0);
  const uint16_t color = effectColor565();
  const int perimeter = 2 * (PANEL_RES_X + PANEL_RES_Y);
  int pos = borderPos % perimeter;
  for (int i = 0; i < 8; i++) {
    const int p = (pos + i) % perimeter;
    int x = 0;
    int y = 0;
    if (p < PANEL_RES_X) {
      x = p;
      y = 0;
    } else if (p < PANEL_RES_X + PANEL_RES_Y) {
      x = PANEL_RES_X - 1;
      y = p - PANEL_RES_X;
    } else if (p < 2 * PANEL_RES_X + PANEL_RES_Y) {
      x = PANEL_RES_X - 1 - (p - PANEL_RES_X - PANEL_RES_Y);
      y = PANEL_RES_Y - 1;
    } else {
      x = 0;
      y = PANEL_RES_Y - 1 - (p - 2 * PANEL_RES_X - PANEL_RES_Y);
    }
    panel->drawPixel(x, y, color);
    if (y + 1 < PANEL_RES_Y) {
      panel->drawPixel(x, y + 1, color);
    }
  }
}
}  // namespace

void effectRendererBegin(VirtualMatrixPanel *virtualPanel) {
  panel = virtualPanel;
}

void effectRendererApply(EffectId id, uint8_t r, uint8_t g, uint8_t b, uint8_t param) {
  activeEffect = id;
  effectColorR = r;
  effectColorG = g;
  effectColorB = b;
  effectParam = param;
  emergencySide = 0;
  emergencyFlashDone = 0;
  arrowPhase = 0;
  lastPhaseMs = millis();
  lastArrowMs = millis();
  lastLifeMs = millis();
  lastStrobeMs = millis();
  lastPulseMs = millis();
  lastBorderMs = millis();
  strobeLit = true;
  pulsePhase = 0;
  borderPos = 0;
  emergencyActive = false;
  panel->fillScreen(0);

  switch (id) {
    case EffectId::BrightWhite:
      panel->fillScreen(effectColor565());
      break;
    case EffectId::BlueEmergency:
    case EffectId::YellowEmergency:
      resetEmergencyState();
      break;
    case EffectId::FullStrobe:
      drawFullStrobe();
      break;
    case EffectId::Pulse:
      drawPulse();
      break;
    case EffectId::BorderChase:
      drawBorderChase();
      break;
    case EffectId::ProgressBar:
      drawProgressBar();
      break;
    case EffectId::GameOfLife:
      seedLife();
      drawLife();
      break;
    case EffectId::ArrowLeft:
      drawArrowSequence(false);
      break;
    case EffectId::ArrowRight:
      drawArrowSequence(true);
      break;
    case EffectId::Stop:
      drawStop();
      break;
    case EffectId::HazardTriangle:
      drawHazardTriangle();
      break;
    default:
      break;
  }
}

bool effectRendererTick(EffectId id) {
  if (!panel || id == EffectId::None) {
    return false;
  }

  const unsigned long now = millis();

  switch (id) {
    case EffectId::BrightWhite:
      return false;

    case EffectId::BlueEmergency:
    case EffectId::YellowEmergency:
      return tickEmergency();

    case EffectId::FullStrobe:
      if (now - lastStrobeMs >= 150) {
        lastStrobeMs = now;
        strobeLit = !strobeLit;
        drawFullStrobe();
        return true;
      }
      return false;

    case EffectId::Pulse:
      if (now - lastPulseMs >= 40) {
        lastPulseMs = now;
        pulsePhase++;
        drawPulse();
        return true;
      }
      return false;

    case EffectId::BorderChase:
      if (now - lastBorderMs >= 60) {
        lastBorderMs = now;
        borderPos = (borderPos + 1) % (2 * (PANEL_RES_X + PANEL_RES_Y));
        drawBorderChase();
        return true;
      }
      return false;

    case EffectId::ProgressBar:
      return false;

    case EffectId::GameOfLife:
      if (now - lastLifeMs >= 200) {
        lastLifeMs = now;
        stepLife();
        drawLife();
        return true;
      }
      return false;

    case EffectId::ArrowLeft:
      if (now - lastArrowMs >= kArrowMs) {
        lastArrowMs = now;
        arrowPhase = (arrowPhase + 1) % kArrowSteps;
        drawArrowSequence(false);
        return true;
      }
      return false;

    case EffectId::ArrowRight:
      if (now - lastArrowMs >= kArrowMs) {
        lastArrowMs = now;
        arrowPhase = (arrowPhase + 1) % kArrowSteps;
        drawArrowSequence(true);
        return true;
      }
      return false;

    case EffectId::Stop:
    case EffectId::HazardTriangle:
      return false;

    default:
      return false;
  }
}

const EffectInfo *effectCatalog(size_t *count) {
  if (count) {
    *count = sizeof(kCatalog) / sizeof(kCatalog[0]);
  }
  return kCatalog;
}

bool effectIsMonochrome(EffectId id) {
  size_t count = 0;
  const EffectInfo *catalog = effectCatalog(&count);
  for (size_t i = 0; i < count; i++) {
    if (catalog[i].effect == id) {
      return catalog[i].monochrome;
    }
  }
  return false;
}

EffectId effectIdFromString(const char *id) {
  if (!id) {
    return EffectId::None;
  }
  if (strcmp(id, "flashing_halves") == 0 || strcmp(id, "blue_emergency") == 0 ||
      strcmp(id, "yellow_emergency") == 0) {
    return EffectId::BlueEmergency;
  }
  size_t count = 0;
  const EffectInfo *catalog = effectCatalog(&count);
  for (size_t i = 0; i < count; i++) {
    if (strcmp(catalog[i].id, id) == 0) {
      return catalog[i].effect;
    }
  }
  return EffectId::None;
}

const char *effectIdToString(EffectId id) {
  if (id == EffectId::BlueEmergency || id == EffectId::YellowEmergency) {
    return "flashing_halves";
  }
  size_t count = 0;
  const EffectInfo *catalog = effectCatalog(&count);
  for (size_t i = 0; i < count; i++) {
    if (catalog[i].effect == id) {
      return catalog[i].id;
    }
  }
  return "none";
}

const char *effectLabel(EffectId id) {
  if (id == EffectId::BlueEmergency || id == EffectId::YellowEmergency) {
    return "Flashing halves";
  }
  size_t count = 0;
  const EffectInfo *catalog = effectCatalog(&count);
  for (size_t i = 0; i < count; i++) {
    if (catalog[i].effect == id) {
      return catalog[i].label;
    }
  }
  return "None";
}
