#include "web_auth.h"

#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <cstring>

namespace {
constexpr char kPrefsNamespace[] = "matrixsign";
constexpr char kPasswordKey[] = "webPass";
constexpr char kUsername[] = "admin";
constexpr char kDefaultPassword[] = "admin";
constexpr size_t kPasswordBufSize = 32;
constexpr size_t kMinPasswordLen = 8;

char passwordBuf[kPasswordBufSize]{};

bool isValidPassword(const char *password) {
  if (!password) {
    return false;
  }
  return strlen(password) >= kMinPasswordLen && strlen(password) < kPasswordBufSize;
}
}  // namespace

void webAuthBegin() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  const bool hadPassword = prefs.isKey(kPasswordKey);
  if (hadPassword) {
    prefs.getString(kPasswordKey, passwordBuf, sizeof(passwordBuf));
  } else {
    strlcpy(passwordBuf, kDefaultPassword, sizeof(passwordBuf));
    prefs.putString(kPasswordKey, passwordBuf);
  }
  prefs.end();

  if (!hadPassword) {
    Serial.println("Web UI login: admin / admin (change under Security in the Web UI)");
  }
}

const char *webAuthUsername() {
  return kUsername;
}

const char *webAuthPassword() {
  return passwordBuf;
}

bool webAuthCheck(AsyncWebServerRequest *request) {
  if (!request) {
    return false;
  }
  if (request->authenticate(kUsername, passwordBuf)) {
    return true;
  }
  request->requestAuthentication(AsyncAuthType::AUTH_BASIC, "MatrixSign");
  return false;
}

bool webAuthSetPassword(const char *password) {
  if (!isValidPassword(password)) {
    return false;
  }
  strlcpy(passwordBuf, password, sizeof(passwordBuf));

  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putString(kPasswordKey, passwordBuf);
  prefs.end();
  return true;
}
