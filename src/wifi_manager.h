#pragma once

#include <Arduino.h>

bool wifiManagerBegin();
void wifiManagerResetAndReboot();
String wifiManagerIp();
int wifiManagerRssi();
