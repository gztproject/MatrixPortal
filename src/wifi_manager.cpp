#include "wifi_manager.h"

#include <WiFi.h>
#include <WiFiManager.h>

namespace {
constexpr char kApSsid[] = "MatrixSign";
const IPAddress kApIp(192, 168, 4, 1);
const IPAddress kApGateway(192, 168, 4, 1);
const IPAddress kApNetmask(255, 255, 255, 0);

bool staAttemptStarted = false;
unsigned long staAttemptStartMs = 0;
constexpr unsigned long kStaAttemptTimeoutMs = 30000;
}  // namespace

bool wifiManagerBegin() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.persistent(true);

  if (!WiFi.softAPConfig(kApIp, kApGateway, kApNetmask)) {
    Serial.println("SoftAP config failed");
    return false;
  }

  if (!WiFi.softAP(kApSsid)) {
    Serial.println("SoftAP start failed");
    return false;
  }

  Serial.printf("AP: %s  http://%s\n", kApSsid, wifiApIp().c_str());

  WiFiManager wm;
  if (wm.getWiFiIsSaved()) {
    WiFi.begin();
    staAttemptStarted = true;
    staAttemptStartMs = millis();
    Serial.println("Attempting saved home Wi-Fi in background...");
  }

  return true;
}

void wifiManagerTick() {
  if (!staAttemptStarted || WiFi.status() == WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECTED && staAttemptStarted) {
      Serial.printf("Home Wi-Fi connected: %s\n", WiFi.localIP().toString().c_str());
      staAttemptStarted = false;
    }
    return;
  }
  if (millis() - staAttemptStartMs > kStaAttemptTimeoutMs) {
    staAttemptStarted = false;
    WiFi.disconnect(true);
    Serial.println("Home Wi-Fi connect timed out");
  }
}

void wifiManagerForgetSta() {
  WiFiManager wm;
  wm.resetSettings();
  WiFi.disconnect(true, true);
  staAttemptStarted = false;
}

bool wifiManagerConnectSta(const char *ssid, const char *password) {
  if (!ssid || ssid[0] == '\0') {
    return false;
  }

  WiFi.begin(ssid, password ? password : "");
  staAttemptStarted = true;
  staAttemptStartMs = millis();
  return true;
}

String wifiApSsid() {
  return String(kApSsid);
}

String wifiApIp() {
  return WiFi.softAPIP().toString();
}

bool wifiStaConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String wifiStaIp() {
  if (!wifiStaConnected()) {
    return String();
  }
  return WiFi.localIP().toString();
}

int wifiStaRssi() {
  if (!wifiStaConnected()) {
    return 0;
  }
  return WiFi.RSSI();
}
