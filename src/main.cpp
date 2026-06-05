#include <Arduino.h>

#include "display_engine.h"
#include "panel_profile.h"
#include "web_server.h"
#include "wifi_manager.h"

DisplayEngine displayEngine;

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

  displayEngine.begin(panelProfileVirtual(), panelProfileDma());

  if (!wifiManagerBegin()) {
    return;
  }

  webServerBegin(displayEngine);
  Serial.println("Ready — open http://matrixsign.local or device IP");
}

void loop() {
  displayEngine.tick();
}
