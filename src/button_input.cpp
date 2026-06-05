#include "button_input.h"

#include "display_engine.h"

namespace {
constexpr int kPinUp = 6;
constexpr int kPinDown = 7;
constexpr unsigned long kDebounceMs = 150;

DisplayEngine *engine = nullptr;
PresetChangeCallback changeCallback = nullptr;
void *changeContext = nullptr;

unsigned long lastUpMs = 0;
unsigned long lastDownMs = 0;
int lastUpState = HIGH;
int lastDownState = HIGH;

void onPresetDelta(int delta) {
  if (!engine) {
    return;
  }
  const int current = engine->activeIndex();
  int next = (current + delta) % PRESET_COUNT;
  if (next < 0) {
    next += PRESET_COUNT;
  }
  engine->selectPreset(next);
  if (changeCallback) {
    changeCallback(next, changeContext);
  }
}

void pollButton(int pin, int &lastState, unsigned long &lastMs, int delta) {
  const int state = digitalRead(pin);
  const unsigned long now = millis();
  if (state == LOW && lastState == HIGH && now - lastMs >= kDebounceMs) {
    lastMs = now;
    onPresetDelta(delta);
  }
  lastState = state;
}
}  // namespace

void buttonInputBegin(DisplayEngine *displayEngine, PresetChangeCallback callback,
                      void *callbackContext) {
  engine = displayEngine;
  changeCallback = callback;
  changeContext = callbackContext;

  pinMode(kPinUp, INPUT_PULLUP);
  pinMode(kPinDown, INPUT_PULLUP);
  lastUpState = digitalRead(kPinUp);
  lastDownState = digitalRead(kPinDown);
}

void buttonInputTick() {
  pollButton(kPinUp, lastUpState, lastUpMs, -1);
  pollButton(kPinDown, lastDownState, lastDownMs, 1);
}
