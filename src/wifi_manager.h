#pragma once

#include <Arduino.h>

bool wifiManagerBegin(bool forceRecoveryAp = false);
void wifiManagerForgetSta();
bool wifiManagerConnectSta(const char *ssid, const char *password);
bool wifiManagerHasSavedSta();
String wifiApSsid();
String wifiApPassword();
String wifiApIp();
bool wifiStaConnected();
String wifiStaSsid();
String wifiStaIp();
int wifiStaRssi();
void wifiManagerTick();
bool wifiManagerApActive();
bool wifiManagerRecoveryMode();
int wifiManagerApClientCount();
const char *wifiManagerModeString();
