#pragma once

#include <Arduino.h>

bool wifiManagerBegin();
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
