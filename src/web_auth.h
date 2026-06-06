#pragma once

#include <Arduino.h>

class AsyncWebServerRequest;

void webAuthBegin();
const char *webAuthUsername();
const char *webAuthPassword();
bool webAuthCheck(AsyncWebServerRequest *request);
bool webAuthSetPassword(const char *password);
