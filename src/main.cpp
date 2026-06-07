#include <Arduino.h>

#include "button_input.h"
#include "display_engine.h"
#include "panel_profile.h"
#include "preset_store.h"
#include "time_sync.h"
#include "ota_update.h"
#include "web_auth.h"
#include "web_server.h"
#include "wifi_manager.h"

#include <FS.h>
#include <LittleFS.h>

PresetStore presetStore;
DisplayEngine displayEngine;

bool initFilesystem() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return false;
  }
  if (!LittleFS.exists("/gif")) {
    LittleFS.mkdir("/gif");
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("MatrixSign — MatrixPortal S3 + P3.076 104x52 13S");

  if (!panelProfileBegin()) {
    Serial.println("Panel init failed");
    while (true) {
      delay(1000);
    }
  }

  if (!initFilesystem()) {
    Serial.println("Filesystem init failed");
  }

  presetStore.begin();
  displayEngine.begin(panelProfileVirtual(), panelProfileDma(), &presetStore);
  buttonInputBegin(&displayEngine);
  timeSyncBegin();
  webAuthBegin();
  otaUpdateBegin();

  if (!wifiManagerBegin()) {
    Serial.println("Wi-Fi init failed");
  }

  webServerBegin(displayEngine, presetStore);
  Serial.println("Ready — connect to MatrixSign, open http://192.168.4.1");
}

void loop() {
  wifiManagerTick();
  buttonInputTick();
  displayEngine.tick();
}
