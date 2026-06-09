#include "button_input.h"

#include "display_engine.h"

namespace {
constexpr int kPinUp = 6;
constexpr int kPinDown = 7;
constexpr unsigned long kDebounceMs = 150;
constexpr unsigned long kHoldMs = 600;
constexpr unsigned long kRepeatMs = 500;
constexpr unsigned long kComboMs = 400;
constexpr int kBrightnessStep = 10;

DisplayEngine *engine = nullptr;
PresetChangeCallback changeCallback = nullptr;
void *changeContext = nullptr;

unsigned long lastUpMs = 0;
unsigned long lastDownMs = 0;
int lastUpState = HIGH;
int lastDownState = HIGH;
unsigned long upHoldStartMs = 0;
unsigned long downHoldStartMs = 0;
bool upHoldHandled = false;
bool downHoldHandled = false;
unsigned long comboStartMs = 0;
bool comboArmed = false;
bool comboHandled = false;

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

void pollButton(int pin, int &lastState, unsigned long &lastMs, int presetDelta, int brightDelta,
                unsigned long &holdStartMs, bool &holdHandled) {
  const int state = digitalRead(pin);
  const unsigned long now = millis();

  if (state == LOW) {
    if (lastState == HIGH) {
      holdStartMs = now;
      holdHandled = false;
    } else if (!holdHandled && now - holdStartMs >= kHoldMs) {
      holdHandled = true;
      lastMs = now;
      if (engine) {
        engine->adjustGlobalBrightness(brightDelta);
      }
    } else if (holdHandled && now - lastMs >= kRepeatMs) {
      lastMs = now;
      if (engine) {
        engine->adjustGlobalBrightness(brightDelta);
      }
    }
  } else if (lastState == LOW && state == HIGH) {
    if (!holdHandled && now - lastMs >= kDebounceMs) {
      lastMs = now;
      onPresetDelta(presetDelta);
    }
    holdHandled = false;
  }

  lastState = state;
}
}  // namespace


bool buttonInputRecoveryHeldAtBoot() {
  pinMode(kPinUp, INPUT_PULLUP);
  pinMode(kPinDown, INPUT_PULLUP);
  const unsigned long deadline = millis() + 200;
  while (millis() < deadline) {
    if (digitalRead(kPinUp) != LOW || digitalRead(kPinDown) != LOW) {
      return false;
    }
    delay(10);
  }
  return digitalRead(kPinUp) == LOW && digitalRead(kPinDown) == LOW;
}

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
  const int upState = digitalRead(kPinUp);
  const int downState = digitalRead(kPinDown);
  const unsigned long now = millis();

  if (upState == LOW && downState == LOW) {
    if (!comboArmed) {
      comboArmed = true;
      comboStartMs = now;
    } else if (!comboHandled && now - comboStartMs >= kComboMs) {
      comboHandled = true;
      if (engine) {
        engine->toggleDisplayOn();
      }
    }
    lastUpState = upState;
    lastDownState = downState;
    return;
  }

  comboArmed = false;
  if (comboHandled) {
    if (upState == HIGH && downState == HIGH) {
      comboHandled = false;
    }
    lastUpState = upState;
    lastDownState = downState;
    return;
  }

  pollButton(kPinUp, lastUpState, lastUpMs, -1, kBrightnessStep, upHoldStartMs, upHoldHandled);
  pollButton(kPinDown, lastDownState, lastDownMs, 1, -kBrightnessStep, downHoldStartMs,
             downHoldHandled);
}
