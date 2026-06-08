#include "wifi_manager.h"

#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <esp_system.h>

#ifndef MATRIXSIGN_AP_PASSWORD
#define MATRIXSIGN_AP_PASSWORD "matrixsign"
#endif

namespace {
constexpr char kApSsid[] = "MatrixSign";
constexpr char kApPassword[] = MATRIXSIGN_AP_PASSWORD;
constexpr char kLegacyWifiNs[] = "wifi";
const IPAddress kApIp(192, 168, 4, 1);
const IPAddress kApGateway(192, 168, 4, 1);
const IPAddress kApNetmask(255, 255, 255, 0);

constexpr unsigned long kStaAttemptTimeoutMs = 12000;
constexpr uint8_t kApChannel = 6;

DNSServer dnsServer;

bool apActive = false;
bool staLinkUp = false;
bool apOnlyMode = true;

void startApDns() {
  dnsServer.start(53, "*", kApIp);
}

void stopApDns() {
  dnsServer.stop();
}

void clearLegacyMultiWifiPrefs() {
  Preferences prefs;
  if (prefs.begin(kLegacyWifiNs, false)) {
    prefs.clear();
    prefs.end();
  }
}

bool startSoftAp() {
  if (!WiFi.softAPConfig(kApIp, kApGateway, kApNetmask)) {
    Serial.println("SoftAP config failed");
    return false;
  }
  if (!WiFi.softAP(kApSsid, kApPassword, kApChannel, 0, 4)) {
    Serial.println("SoftAP start failed");
    return false;
  }
  delay(500);
  return true;
}

void stopSoftAp() {
  if (!apActive) {
    return;
  }
  stopApDns();
  WiFi.softAPdisconnect(true);
  apActive = false;
  Serial.println("AP disabled (home Wi-Fi connected)");
}

bool startApMode() {
  if (WiFi.status() == WL_CONNECTED) {
    return false;
  }

  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);

  if (!startSoftAp()) {
    return false;
  }

  apActive = true;
  apOnlyMode = true;
  staLinkUp = false;
  startApDns();
  Serial.printf("AP: %s ch%d (WPA2)  http://%s\n", kApSsid, kApChannel,
                WiFi.softAPIP().toString().c_str());
  return true;
}

bool tryConnectSavedStaBlocking() {
  WiFiManager wm;
  if (!wm.getWiFiIsSaved()) {
    return false;
  }

  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(true);
  WiFi.begin();

  Serial.printf("Connecting to saved home Wi-Fi: %s\n", wm.getWiFiSSID(true).c_str());

  const unsigned long deadline = millis() + kStaAttemptTimeoutMs;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    delay(50);
  }

  if (WiFi.status() == WL_CONNECTED) {
    apOnlyMode = false;
    return true;
  }

  Serial.println("Home Wi-Fi unavailable at boot");
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  delay(100);
  return false;
}
}  // namespace

bool wifiManagerBegin() {
  clearLegacyMultiWifiPrefs();

  WiFi.setSleep(false);
  WiFi.setAutoReconnect(false);

  if (tryConnectSavedStaBlocking()) {
    staLinkUp = true;
    apActive = false;
    apOnlyMode = false;
    Serial.printf("Home Wi-Fi connected: %s (%s)\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    return true;
  }

  if (!startApMode()) {
    return false;
  }

  Serial.println("AP-only — STA disabled until reboot or Connect");
  return true;
}

void wifiManagerTick() {
  if (apOnlyMode) {
    if (apActive) {
      dnsServer.processNextRequest();
    }
    return;
  }

  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    if (!staLinkUp) {
      Serial.printf("Home Wi-Fi connected: %s (%s)\n", WiFi.SSID().c_str(),
                    WiFi.localIP().toString().c_str());
      staLinkUp = true;
      stopSoftAp();
    }
    return;
  }

  if (staLinkUp) {
    staLinkUp = false;
    WiFi.disconnect(false);
    WiFi.mode(WIFI_OFF);
    delay(100);
    startApMode();
    Serial.println("Home Wi-Fi lost — AP restored, STA disabled");
  }
}

void wifiManagerForgetSta() {
  clearLegacyMultiWifiPrefs();

  WiFiManager wm;
  wm.resetSettings();
  staLinkUp = false;
  apActive = false;
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  startApMode();
}

bool wifiManagerHasSavedSta() {
  WiFiManager wm;
  return wm.getWiFiIsSaved();
}

bool wifiManagerConnectSta(const char *ssid, const char *password) {
  if (!ssid || ssid[0] == '\0') {
    return false;
  }

  clearLegacyMultiWifiPrefs();
  WiFi.persistent(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.begin(ssid, password ? password : "");
  delay(300);
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);

  Serial.println("Home Wi-Fi saved — rebooting to connect");
  delay(100);
  esp_restart();
  return false;
}

String wifiApSsid() {
  return String(kApSsid);
}

String wifiApPassword() {
  return String(kApPassword);
}

String wifiApIp() {
  if (!apActive) {
    return String();
  }
  return WiFi.softAPIP().toString();
}

bool wifiStaConnected() {
  return !apOnlyMode && WiFi.status() == WL_CONNECTED;
}

String wifiStaSsid() {
  if (!wifiStaConnected()) {
    return String();
  }
  return WiFi.SSID();
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
