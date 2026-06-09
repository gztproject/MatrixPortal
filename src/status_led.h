#pragma once

#include <Arduino.h>

enum class WifiLedState : uint8_t {
  Off,
  YellowBlink,  // saved credentials, searching at boot
  GreenBlink,   // STA connecting / reconnecting
  GreenSolid,   // STA connected
  RedBlink,     // AP up, waiting for client
  RedSolid,     // AP client connected
};

void statusLedBegin();
void statusLedSet(WifiLedState state);
void statusLedTick();
