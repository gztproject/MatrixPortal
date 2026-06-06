#include "web_auth.h"

#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <cstring>
#include <esp_random.h>

namespace {
constexpr char kPrefsNamespace[] = "matrixsign";
constexpr char kPasswordKey[] = "webPass";
constexpr char kUsername[] = "admin";
constexpr size_t kPasswordLen = 12;
constexpr size_t kPasswordBufSize = 32;
constexpr size_t kMinPasswordLen = 8;

constexpr char kPasswordAlphabet[] =
    "abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789";

char passwordBuf[kPasswordBufSize]{};

void generateRandomPassword() {
  for (size_t i = 0; i < kPasswordLen; i++) {
    const uint32_t r = esp_random();
    passwordBuf[i] = kPasswordAlphabet[r % (sizeof(kPasswordAlphabet) - 1)];
  }
  passwordBuf[kPasswordLen] = '\0';
}

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
    generateRandomPassword();
    prefs.putString(kPasswordKey, passwordBuf);
  }
  prefs.end();

  if (!hadPassword) {
    Serial.println("=== MatrixSign Web UI (save this password) ===");
    Serial.printf("  Username: %s\n", kUsername);
    Serial.printf("  Password: %s\n", passwordBuf);
    Serial.println("=============================================");
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
  return request->authenticate(kUsername, passwordBuf);
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
