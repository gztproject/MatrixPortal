#include "web_server.h"

#include "web_ui.h"
#include "wifi_manager.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

namespace {
AsyncWebServer server(80);
DisplayEngine *displayEngine = nullptr;

uint32_t parseHexColor(const char *hex) {
  if (!hex || hex[0] != '#') {
    return 0xFFFFFF;
  }
  return strtoul(hex + 1, nullptr, 16) & 0xFFFFFF;
}

void configToJson(const SignConfig &cfg, JsonDocument &doc) {
  doc["text"] = cfg.text;
  doc["scroll"] = cfg.scroll;
  doc["scrollDelayMs"] = cfg.scrollDelayMs;
  doc["brightness"] = cfg.brightness;

  char color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", cfg.colorR, cfg.colorG, cfg.colorB);
  doc["color"] = color;
  doc["ip"] = wifiManagerIp();
  doc["rssi"] = wifiManagerRssi();
}

bool jsonToConfig(JsonObject obj, SignConfig &cfg) {
  if (obj["text"].is<const char *>()) {
    strlcpy(cfg.text, obj["text"], sizeof(cfg.text));
  }
  if (obj["scroll"].is<bool>()) {
    cfg.scroll = obj["scroll"];
  }
  if (obj["scrollDelayMs"].is<uint16_t>()) {
    cfg.scrollDelayMs = obj["scrollDelayMs"];
  } else if (obj["scrollDelayMs"].is<int>()) {
    cfg.scrollDelayMs = static_cast<uint16_t>(obj["scrollDelayMs"].as<int>());
  }
  if (obj["brightness"].is<uint8_t>()) {
    cfg.brightness = obj["brightness"];
  } else if (obj["brightness"].is<int>()) {
    cfg.brightness = static_cast<uint8_t>(obj["brightness"].as<int>());
  }
  if (obj["color"].is<const char *>()) {
    const uint32_t rgb = parseHexColor(obj["color"]);
    cfg.colorR = (rgb >> 16) & 0xFF;
    cfg.colorG = (rgb >> 8) & 0xFF;
    cfg.colorB = rgb & 0xFF;
  }
  return true;
}
}  // namespace

void webServerBegin(DisplayEngine &engine) {
  displayEngine = &engine;

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", WEB_UI_HTML);
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    configToJson(displayEngine->getConfig(), doc);
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on(
    "/api/config",
    HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      const DeserializationError err = deserializeJson(doc, data, len);
      if (err) {
        request->send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
      }

      SignConfig cfg = displayEngine->getConfig();
      if (!jsonToConfig(doc.as<JsonObject>(), cfg)) {
        request->send(400, "application/json", "{\"error\":\"invalid config\"}");
        return;
      }

      displayEngine->applyConfig(cfg);
      request->send(200, "application/json", "{\"ok\":true}");
    });

  server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
    delay(200);
    wifiManagerResetAndReboot();
  });

  server.begin();
  Serial.println("Web server started on port 80");
}
