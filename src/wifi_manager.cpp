#include "wifi_manager.h"

#include "status_led.h"
#include "wifi_config.h"

#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_wifi.h>

#ifndef MATRIXSIGN_AP_PASSWORD
#define MATRIXSIGN_AP_PASSWORD "matrixsign"
#endif

namespace {
constexpr char kApPassword[] = MATRIXSIGN_AP_PASSWORD;
constexpr char kLegacyWifiNs[] = "wifi";
const IPAddress kApIp(192, 168, 4, 1);
const IPAddress kApGateway(192, 168, 4, 1);
const IPAddress kApNetmask(255, 255, 255, 0);

enum class WifiState : uint8_t { ApFallback, StaConnected, StaLostRetrying };

DNSServer dnsServer;

WifiState state = WifiState::ApFallback;
bool apActive = false;
bool staLinkUp = false;
bool recoveryMode = false;
unsigned long staLossStartMs = 0;
char apSsid[24] = "MatrixSign";

void buildApSsid() {
  uint8_t mac[6]{};
  esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
  snprintf(apSsid, sizeof(apSsid), "MatrixSign-%02X%02X", mac[4], mac[5]);
}

void refreshStatusLed() {
  if (apActive) {
    statusLedSet(wifiManagerApClientCount() > 0 ? WifiLedState::RedSolid : WifiLedState::RedBlink);
    return;
  }
  if (state == WifiState::StaConnected && WiFi.status() == WL_CONNECTED) {
    statusLedSet(WifiLedState::GreenSolid);
    return;
  }
  if (state == WifiState::StaLostRetrying) {
    statusLedSet(WifiLedState::GreenBlink);
  }
}

void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("wifi: STA got IP %s\n", WiFi.localIP().toString().c_str());
      refreshStatusLed();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.printf("wifi: STA disconnected reason %d\n", info.wifi_sta_disconnected.reason);
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.printf("wifi: AP client joined (%d)\n", WiFi.softAPgetStationNum());
      refreshStatusLed();
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.printf("wifi: AP client left (%d)\n", WiFi.softAPgetStationNum());
      refreshStatusLed();
      break;
    default:
      break;
  }
}

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
    Serial.println("wifi: SoftAP config failed");
    return false;
  }
  if (!WiFi.softAP(apSsid, kApPassword, MATRIXSIGN_AP_CHANNEL, 0, MATRIXSIGN_AP_MAX_CLIENTS)) {
    Serial.println("wifi: SoftAP start failed");
    return false;
  }
  delay(300);
  return true;
}

void stopSoftAp() {
  if (!apActive) {
    return;
  }
  stopApDns();
  WiFi.softAPdisconnect(true);
  apActive = false;
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
  WiFi.setTxPower(WIFI_POWER_15dBm);

  if (!startSoftAp()) {
    return false;
  }

  apActive = true;
  staLinkUp = false;
  state = WifiState::ApFallback;
  startApDns();
  Serial.printf("wifi: AP %s ch%d http://%s\n", apSsid, MATRIXSIGN_AP_CHANNEL,
                WiFi.softAPIP().toString().c_str());
  refreshStatusLed();
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
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin();

  Serial.printf("wifi: connecting %s\n", wm.getWiFiSSID(true).c_str());
  statusLedSet(WifiLedState::YellowBlink);

  const unsigned long deadline = millis() + MATRIXSIGN_STA_BOOT_TIMEOUT_MS;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    statusLedTick();
    delay(50);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    statusLedSet(WifiLedState::GreenSolid);
    return true;
  }

  Serial.println("wifi: STA unavailable at boot — AP only (credentials kept)");
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  delay(100);
  return false;
}
}  // namespace

bool wifiManagerBegin(bool forceRecoveryAp) {
  clearLegacyMultiWifiPrefs();
  buildApSsid();
  recoveryMode = forceRecoveryAp;

  WiFi.onEvent(onWifiEvent);
  WiFi.setSleep(false);

  if (forceRecoveryAp) {
    Serial.println("wifi: safe mode — STA off, AP only");
    WiFi.mode(WIFI_OFF);
    delay(50);
    statusLedSet(WifiLedState::RedBlink);
    if (!startApMode()) {
      return false;
    }
    return true;
  }

  if (tryConnectSavedStaBlocking()) {
    staLinkUp = true;
    apActive = false;
    state = WifiState::StaConnected;
    Serial.printf("wifi: STA connected %s (%s)\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    refreshStatusLed();
    return true;
  }

  if (!startApMode()) {
    return false;
  }

  Serial.println("wifi: AP-only fallback");
  return true;
}

void wifiManagerTick() {
  if (state == WifiState::ApFallback) {
    if (apActive) {
      dnsServer.processNextRequest();
      refreshStatusLed();
    }
    return;
  }

  const bool connected = WiFi.status() == WL_CONNECTED;

  if (state == WifiState::StaConnected) {
    if (connected) {
      staLinkUp = true;
      return;
    }

    staLinkUp = false;
    staLossStartMs = millis();
    state = WifiState::StaLostRetrying;
    Serial.println("wifi: STA lost — retrying");
    statusLedSet(WifiLedState::GreenBlink);
    WiFi.reconnect();
    return;
  }

  if (state == WifiState::StaLostRetrying) {
    if (connected) {
      staLinkUp = true;
      state = WifiState::StaConnected;
      Serial.printf("wifi: STA reconnected %s\n", WiFi.localIP().toString().c_str());
      refreshStatusLed();
      return;
    }

    if (millis() - staLossStartMs < MATRIXSIGN_STA_LOSS_RETRY_MS) {
      return;
    }

    Serial.println("wifi: STA retry expired — AP fallback");
    WiFi.disconnect(false);
    WiFi.mode(WIFI_OFF);
    delay(100);
    startApMode();
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

  Serial.println("wifi: credentials saved — rebooting");
  delay(100);
  esp_restart();
  return false;
}

String wifiApSsid() {
  return String(apSsid);
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
  return state == WifiState::StaConnected && WiFi.status() == WL_CONNECTED;
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

bool wifiManagerApActive() {
  return apActive;
}

bool wifiManagerRecoveryMode() {
  return recoveryMode;
}

int wifiManagerApClientCount() {
  if (!apActive) {
    return 0;
  }
  return WiFi.softAPgetStationNum();
}

const char *wifiManagerModeString() {
  if (recoveryMode && apActive) {
    return "RECOVERY_AP";
  }
  if (apActive) {
    return "AP";
  }
  if (state == WifiState::StaLostRetrying) {
    return "STA_RETRY";
  }
  if (wifiStaConnected()) {
    return "STA";
  }
  return "OFF";
}
