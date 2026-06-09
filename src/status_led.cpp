#include "status_led.h"

namespace {
constexpr unsigned long kBlinkMs = 500;
constexpr uint8_t kLedLevel = 48;

WifiLedState currentState = WifiLedState::Off;
unsigned long lastToggleMs = 0;
bool blinkOn = false;

void writeRgb(uint8_t r, uint8_t g, uint8_t b) {
#if defined(RGB_BUILTIN)
  neopixelWrite(RGB_BUILTIN, r, g, b);
#elif defined(NEOPIXEL_PIN)
  neopixelWrite(NEOPIXEL_PIN, r, g, b);
#else
  (void)r;
  (void)g;
  (void)b;
#endif
}

void applyColor(uint8_t r, uint8_t g, uint8_t b) {
  writeRgb(r, g, b);
}
}  // namespace

void statusLedBegin() {
#if defined(RGB_BUILTIN) || defined(NEOPIXEL_PIN)
  writeRgb(0, 0, 0);
#endif
  currentState = WifiLedState::Off;
  lastToggleMs = millis();
  blinkOn = false;
}

void statusLedSet(WifiLedState state) {
  if (currentState == state) {
    return;
  }
  currentState = state;
  lastToggleMs = millis();
  blinkOn = true;

  switch (state) {
    case WifiLedState::GreenSolid:
      applyColor(0, kLedLevel, 0);
      break;
    case WifiLedState::RedSolid:
      applyColor(kLedLevel, 0, 0);
      break;
    case WifiLedState::Off:
      applyColor(0, 0, 0);
      break;
    default:
      break;
  }
}

void statusLedTick() {
  const unsigned long now = millis();
  if (now - lastToggleMs < kBlinkMs) {
    return;
  }
  lastToggleMs = now;
  blinkOn = !blinkOn;

  switch (currentState) {
    case WifiLedState::YellowBlink:
      applyColor(blinkOn ? kLedLevel : 0, blinkOn ? kLedLevel : 0, 0);
      break;
    case WifiLedState::GreenBlink:
      applyColor(0, blinkOn ? kLedLevel : 0, 0);
      break;
    case WifiLedState::RedBlink:
      applyColor(blinkOn ? kLedLevel : 0, 0, 0);
      break;
    default:
      break;
  }
}
