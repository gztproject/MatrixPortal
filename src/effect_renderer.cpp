#include "effect_renderer.h"

#include "panel_profile.h"

namespace {
VirtualMatrixPanel *panel = nullptr;
EffectId activeEffect = EffectId::None;
unsigned long lastPhaseMs = 0;
uint8_t emergencyPhase = 0;
uint8_t arrowPhase = 0;
unsigned long lastArrowMs = 0;

constexpr unsigned long kStrobeMs = 350;
constexpr unsigned long kArrowMs = 120;
constexpr int kChevronWidth = PANEL_RES_Y / 2;
constexpr int kArrowSteps = 8;
constexpr int kArrowStepX = (PANEL_RES_X - kChevronWidth) / (kArrowSteps - 1);

static const EffectInfo kCatalog[] = {
    {"bright_white", "Bright white", EffectId::BrightWhite},
    {"blue_emergency", "Blue emergency", EffectId::BlueEmergency},
    {"yellow_emergency", "Yellow emergency", EffectId::YellowEmergency},
    {"arrow_left", "Arrows left", EffectId::ArrowLeft},
    {"arrow_right", "Arrows right", EffectId::ArrowRight},
    {"stop", "Stop", EffectId::Stop},
    {"hazard_triangle", "Hazard triangle", EffectId::HazardTriangle},
};

void drawEmergency(uint8_t r, uint8_t g, uint8_t b, uint8_t phase) {
  const uint16_t color = panel->color565(r, g, b);
  const int halfWidth = PANEL_RES_X / 2;

  panel->fillScreen(0);
  if (phase == 0) {
    panel->fillRect(0, 0, halfWidth, PANEL_RES_Y, color);
  } else {
    panel->fillRect(halfWidth, 0, PANEL_RES_X - halfWidth, PANEL_RES_Y, color);
  }
}

void drawFullHeightChevron(bool right, int x, uint16_t color) {
  const int halfHeight = PANEL_RES_Y / 2;
  for (int y = 0; y < PANEL_RES_Y; y++) {
    const int span = (kChevronWidth * abs(y - halfHeight)) / halfHeight;
    if (right) {
      const int x0 = x + kChevronWidth - span;
      panel->drawFastHLine(x0, y, span + 1, color);
    } else {
      panel->drawFastHLine(x, y, span + 1, color);
    }
  }
}

void drawArrowSequence(bool right) {
  panel->fillScreen(0);
  const uint16_t color = panel->color565(255, 255, 0);

  const int phase = arrowPhase % kArrowSteps;
  const int x =
      right ? phase * kArrowStepX : (PANEL_RES_X - kChevronWidth - phase * kArrowStepX);
  drawFullHeightChevron(right, x, color);
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
  panel->setCursor((PANEL_RES_X - textWidth) / 2, (PANEL_RES_Y - textHeight) / 2);
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
}  // namespace

void effectRendererBegin(VirtualMatrixPanel *virtualPanel) {
  panel = virtualPanel;
}

void effectRendererApply(EffectId id) {
  activeEffect = id;
  emergencyPhase = 0;
  arrowPhase = 0;
  lastPhaseMs = millis();
  lastArrowMs = millis();
  panel->fillScreen(0);

  switch (id) {
    case EffectId::BrightWhite:
      panel->fillScreen(panel->color565(255, 255, 255));
      break;
    case EffectId::BlueEmergency:
      drawEmergency(0, 0, 255, emergencyPhase);
      break;
    case EffectId::YellowEmergency:
      drawEmergency(255, 170, 0, emergencyPhase);
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
      if (now - lastPhaseMs >= kStrobeMs) {
        lastPhaseMs = now;
        emergencyPhase ^= 1;
        drawEmergency(0, 0, 255, emergencyPhase);
      }
      return true;

    case EffectId::YellowEmergency:
      if (now - lastPhaseMs >= kStrobeMs) {
        lastPhaseMs = now;
        emergencyPhase ^= 1;
        drawEmergency(255, 170, 0, emergencyPhase);
      }
      return true;

    case EffectId::ArrowLeft:
      if (now - lastArrowMs >= kArrowMs) {
        lastArrowMs = now;
        arrowPhase = (arrowPhase + 1) % kArrowSteps;
        drawArrowSequence(false);
      }
      return true;

    case EffectId::ArrowRight:
      if (now - lastArrowMs >= kArrowMs) {
        lastArrowMs = now;
        arrowPhase = (arrowPhase + 1) % kArrowSteps;
        drawArrowSequence(true);
      }
      return true;

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

EffectId effectIdFromString(const char *id) {
  if (!id) {
    return EffectId::None;
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
  size_t count = 0;
  const EffectInfo *catalog = effectCatalog(&count);
  for (size_t i = 0; i < count; i++) {
    if (catalog[i].effect == id) {
      return catalog[i].label;
    }
  }
  return "None";
}
