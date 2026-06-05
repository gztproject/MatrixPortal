#include "wifi_manager.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiManager.h>

namespace {
constexpr char kApName[] = "MatrixSign-Setup";
constexpr char kHostname[] = "matrixsign";
}  // namespace

bool wifiManagerBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(30);

  Serial.printf("Connecting to Wi-Fi (portal AP: %s)...\n", kApName);
  const bool connected = wm.autoConnect(kApName);

  if (!connected) {
    Serial.println("Wi-Fi connect failed, restarting...");
    delay(3000);
    ESP.restart();
    return false;
  }

  Serial.printf("Wi-Fi connected: %s\n", WiFi.SSID().c_str());
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

  if (MDNS.begin(kHostname)) {
    Serial.printf("mDNS: http://%s.local\n", kHostname);
  } else {
    Serial.println("mDNS init failed");
  }

  return true;
}

void wifiManagerResetAndReboot() {
  WiFiManager wm;
  wm.resetSettings();
  delay(500);
  ESP.restart();
}

String wifiManagerIp() {
  return WiFi.localIP().toString();
}

int wifiManagerRssi() {
  return WiFi.RSSI();
}
