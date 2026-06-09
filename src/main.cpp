#include <Arduino.h>
#include <esp_system.h>

#include "button_input.h"
#include "display_engine.h"
#include "panel_profile.h"
#include "preset_store.h"
#include "time_sync.h"
#include "ota_update.h"
#include "web_auth.h"
#include "web_server.h"
#include "status_led.h"
#include "wifi_manager.h"

#include <FS.h>
#include <LittleFS.h>

PresetStore presetStore;
DisplayEngine displayEngine;

void logBootMilestone(const char *msg) {
  Serial.printf("boot: %s (heap %u min %u)\n", msg, ESP.getFreeHeap(), ESP.getMinFreeHeap());
}

const char *espResetReasonString() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
      return "poweron";
    case ESP_RST_EXT:
      return "ext";
    case ESP_RST_SW:
      return "sw";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "int_wdt";
    case ESP_RST_TASK_WDT:
      return "task_wdt";
    case ESP_RST_WDT:
      return "wdt";
    case ESP_RST_DEEPSLEEP:
      return "deepsleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    default:
      return "unknown";
  }
}

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
  statusLedBegin();
  Serial.printf("boot: reset_reason=%s\n", espResetReasonString());
  logBootMilestone("start");

  const bool recoveryBoot = buttonInputRecoveryHeldAtBoot();

  if (recoveryBoot) {
    Serial.println("boot: safe mode — AP first, no STA");
    if (!initFilesystem()) {
      Serial.println("Filesystem init failed");
    }
    logBootMilestone("fs ok");
    presetStore.begin();
    webAuthBegin();
    if (!wifiManagerBegin(true)) {
      Serial.println("Wi-Fi init failed");
    }
    logBootMilestone("wifi ap ok");
    webServerBegin(displayEngine, presetStore);
    logBootMilestone("web ok");
    if (!panelProfileBegin()) {
      Serial.println("Panel init failed");
      while (true) {
        delay(1000);
      }
    }
    logBootMilestone("panel ok");
    displayEngine.beginRecoveryMode(panelProfileVirtual(), panelProfileDma(), &presetStore);
  } else {
    if (!panelProfileBegin()) {
      Serial.println("Panel init failed");
      while (true) {
        delay(1000);
      }
    }
    logBootMilestone("panel ok");
    if (!initFilesystem()) {
      Serial.println("Filesystem init failed");
    }
    logBootMilestone("fs ok");
    presetStore.begin();
    webAuthBegin();
    if (!wifiManagerBegin(false)) {
      Serial.println("Wi-Fi init failed");
    }
    logBootMilestone("wifi ok");
    webServerBegin(displayEngine, presetStore);
    logBootMilestone("web ok");
    if (esp_reset_reason() == ESP_RST_BROWNOUT) {
      displayEngine.setBrownoutBootClamp(true);
      Serial.println("boot: brownout reset, brightness capped 10% (raise via UI to clear)");
    }
    displayEngine.begin(panelProfileVirtual(), panelProfileDma(), &presetStore);
  }

  buttonInputBegin(&displayEngine);
  timeSyncBegin();
  otaUpdateBegin();

  Serial.printf("boot: mode=%s ap=%s sta=%s\n", wifiManagerModeString(),
                wifiManagerApActive() ? wifiApSsid().c_str() : "-",
                wifiStaConnected() ? wifiStaIp().c_str() : "-");
  logBootMilestone("ready");
}

void loop() {
  wifiManagerTick();
  statusLedTick();
  buttonInputTick();
  displayEngine.tick();
}
