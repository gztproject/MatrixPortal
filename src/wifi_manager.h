#pragma once

#include <Arduino.h>

bool wifiManagerBegin();
void wifiManagerForgetSta();
bool wifiManagerConnectSta(const char *ssid, const char *password);
String wifiApSsid();
String wifiApIp();
bool wifiStaConnected();
String wifiStaIp();
int wifiStaRssi();
void wifiManagerTick();
