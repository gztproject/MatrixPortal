#include "web_server.h"

#include "effect_renderer.h"
#include "web_ui.h"
#include "wifi_manager.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>

namespace {
AsyncWebServer server(80);
DisplayEngine *displayEngine = nullptr;
PresetStore *presetStore = nullptr;

File uploadFile;
int uploadPresetId = -1;
size_t uploadTotalBytes = 0;

uint32_t parseHexColor(const char *hex) {
  if (!hex || hex[0] != '#') {
    return 0xFFFFFF;
  }
  return strtoul(hex + 1, nullptr, 16) & 0xFFFFFF;
}

void appendWifiStatus(JsonObject obj) {
  obj["apSsid"] = wifiApSsid();
  obj["apPassword"] = wifiApPassword();
  obj["apIp"] = wifiApIp();
  obj["staConnected"] = wifiStaConnected();
  if (wifiStaConnected()) {
    obj["staIp"] = wifiStaIp();
    obj["staRssi"] = wifiStaRssi();
  } else {
    obj["staIp"] = "";
    obj["staRssi"] = nullptr;
  }
}

void appendGlobalBrightness(JsonObject obj) {
  obj["brightness"] = presetStore->globalBrightness();
}

void presetToJson(const SignPreset &preset, JsonObject obj, int index) {
  obj["contentType"] = contentTypeToString(preset.contentType);
  obj["effectId"] = effectIdToString(static_cast<EffectId>(preset.effectId));
  obj["effectLabel"] = effectLabel(static_cast<EffectId>(preset.effectId));
  obj["textHeightPx"] = preset.textHeightPx;
  obj["rowCount"] = preset.rowCount;
  obj["text"] = preset.text;
  obj["scroll"] = preset.scroll;
  obj["scrollDelayMs"] = preset.scrollDelayMs;

  char color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", preset.colorR, preset.colorG, preset.colorB);
  obj["color"] = color;
  obj["gifPath"] = preset.gifPath;
  obj["hasGif"] = presetStore->gifExistsForSlot(index);
}

bool jsonToPreset(JsonObject obj, SignPreset &preset) {
  if (obj["contentType"].is<const char *>()) {
    preset.contentType = contentTypeFromString(obj["contentType"]);
  }
  if (obj["effectId"].is<const char *>()) {
    preset.effectId = static_cast<uint8_t>(effectIdFromString(obj["effectId"]));
  } else if (obj["effectId"].is<int>()) {
    preset.effectId = static_cast<uint8_t>(obj["effectId"].as<int>());
  }
  if (obj["textHeightPx"].is<int>()) {
    preset.textHeightPx = clampTextHeightPx(obj["textHeightPx"].as<int>());
  } else if (obj["fontScale"].is<const char *>()) {
    preset.textHeightPx = textHeightPxFromLegacyFontScale(obj["fontScale"]);
  }
  if (obj["rowCount"].is<int>()) {
    preset.rowCount = static_cast<uint8_t>(obj["rowCount"].as<int>());
  } else if (obj["rowCount"].is<uint8_t>()) {
    preset.rowCount = obj["rowCount"];
  }
  if (obj["text"].is<const char *>()) {
    strlcpy(preset.text, obj["text"], sizeof(preset.text));
  }
  if (obj["scroll"].is<bool>()) {
    preset.scroll = obj["scroll"];
  }
  if (obj["scrollDelayMs"].is<uint16_t>()) {
    preset.scrollDelayMs = obj["scrollDelayMs"];
  } else if (obj["scrollDelayMs"].is<int>()) {
    preset.scrollDelayMs = static_cast<uint16_t>(obj["scrollDelayMs"].as<int>());
  }
  if (obj["color"].is<const char *>()) {
    const uint32_t rgb = parseHexColor(obj["color"]);
    preset.colorR = (rgb >> 16) & 0xFF;
    preset.colorG = (rgb >> 8) & 0xFF;
    preset.colorB = rgb & 0xFF;
  }
  if (preset.rowCount < 1) {
    preset.rowCount = 1;
  }
  if (preset.rowCount > 4) {
    preset.rowCount = 4;
  }
  return true;
}

void sendPresetsJson(AsyncWebServerRequest *request) {
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  root["activeIndex"] = presetStore->activeIndex();
  JsonArray arr = root["presets"].to<JsonArray>();
  for (int i = 0; i < PRESET_COUNT; i++) {
    JsonObject item = arr.add<JsonObject>();
    presetToJson(presetStore->get(i), item, i);
  }
  appendGlobalBrightness(root);
  appendWifiStatus(root);

  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

void appendPresetFields(JsonObject obj, const SignPreset &preset) {
  obj["contentType"] = contentTypeToString(preset.contentType);
  obj["effectId"] = effectIdToString(static_cast<EffectId>(preset.effectId));
  obj["effectLabel"] = effectLabel(static_cast<EffectId>(preset.effectId));
  obj["textHeightPx"] = preset.textHeightPx;
  obj["rowCount"] = preset.rowCount;
  obj["text"] = preset.text;
  obj["scroll"] = preset.scroll;
  obj["scrollDelayMs"] = preset.scrollDelayMs;
  char color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", preset.colorR, preset.colorG, preset.colorB);
  obj["color"] = color;
}
}  // namespace

void webServerBegin(DisplayEngine &engine, PresetStore &store) {
  displayEngine = &engine;
  presetStore = &store;

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", WEB_UI_HTML);
  });

  server.on("/api/effects", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray arr = doc["effects"].to<JsonArray>();
    size_t count = 0;
    const EffectInfo *catalog = effectCatalog(&count);
    for (size_t i = 0; i < count; i++) {
      JsonObject item = arr.add<JsonObject>();
      item["id"] = catalog[i].id;
      item["label"] = catalog[i].label;
    }
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/presets/select", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const int id = doc["id"] | -1;
              if (id < 0 || id >= PRESET_COUNT) {
                request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
                return;
              }
              displayEngine->selectPreset(id);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/brightness", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["brightness"] = presetStore->globalBrightness();
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const int brightness = doc["brightness"] | -1;
              if (brightness < 1 || brightness > 100) {
                request->send(400, "application/json", "{\"error\":\"brightness must be 1-100\"}");
                return;
              }
              displayEngine->setGlobalBrightness(static_cast<uint8_t>(brightness));
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/presets", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendPresetsJson(request);
  });

  server.on(AsyncURIMatcher::exact("/api/presets"), HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              if (index + len == total) {
                JsonDocument doc;
                if (deserializeJson(doc, data, len)) {
                  request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                  return;
                }
                const int id = doc["id"] | presetStore->activeIndex();
                if (id < 0 || id >= PRESET_COUNT) {
                  request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
                  return;
                }
                SignPreset preset = presetStore->get(id);
                if (!jsonToPreset(doc.as<JsonObject>(), preset)) {
                  request->send(400, "application/json", "{\"error\":\"invalid preset\"}");
                  return;
                }
                displayEngine->applyPreset(preset, id);
                request->send(200, "application/json", "{\"ok\":true}");
              }
            });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    const SignPreset preset = displayEngine->activePreset();
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    appendPresetFields(root, preset);
    root["activeIndex"] = displayEngine->activeIndex();
    appendGlobalBrightness(root);
    appendWifiStatus(root);

    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              SignPreset preset = displayEngine->activePreset();
              if (!jsonToPreset(doc.as<JsonObject>(), preset)) {
                request->send(400, "application/json", "{\"error\":\"invalid config\"}");
                return;
              }
              displayEngine->applyPreset(preset, displayEngine->activeIndex());
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    wifiManagerForgetSta();
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              if (index + len != total) {
                return;
              }
              JsonDocument doc;
              if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
              }
              const char *ssid = doc["ssid"] | "";
              const char *password = doc["password"] | "";
              if (ssid[0] == '\0') {
                request->send(400, "application/json", "{\"error\":\"ssid required\"}");
                return;
              }
              wifiManagerConnectSta(ssid, password);
              request->send(200, "application/json", "{\"ok\":true}");
            });

  server.on("/api/presets/gif", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    const int id = request->hasParam("id") ? request->getParam("id")->value().toInt() : -1;
    if (id < 0 || id >= PRESET_COUNT) {
      request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
      return;
    }
    const String path = presetStore->gifPathForSlot(id);
    if (LittleFS.exists(path)) {
      LittleFS.remove(path);
    }
    SignPreset preset = presetStore->get(id);
    path.toCharArray(preset.gifPath, sizeof(preset.gifPath));
    if (preset.contentType == ContentType::Gif) {
      preset.contentType = ContentType::Text;
    }
    displayEngine->applyPreset(preset, id);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on(
      "/api/presets/gif",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {
        if (uploadPresetId < 0 || uploadPresetId >= PRESET_COUNT) {
          request->send(400, "application/json", "{\"error\":\"invalid preset id\"}");
          return;
        }
        if (uploadTotalBytes > MAX_GIF_BYTES) {
          const String path = presetStore->gifPathForSlot(uploadPresetId);
          if (LittleFS.exists(path)) {
            LittleFS.remove(path);
          }
          request->send(413, "application/json", "{\"error\":\"gif too large (max 256KB)\"}");
          return;
        }
        SignPreset preset = presetStore->get(uploadPresetId);
        const String path = presetStore->gifPathForSlot(uploadPresetId);
        path.toCharArray(preset.gifPath, sizeof(preset.gifPath));
        preset.contentType = ContentType::Gif;
        displayEngine->applyPreset(preset, uploadPresetId);
        request->send(200, "application/json", "{\"ok\":true}");
      },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len,
         bool final) {
        if (index == 0) {
          uploadPresetId = request->hasParam("id") ? request->getParam("id")->value().toInt() : -1;
          uploadTotalBytes = 0;
          if (uploadPresetId < 0 || uploadPresetId >= PRESET_COUNT) {
            return;
          }
          const String path = presetStore->gifPathForSlot(uploadPresetId);
          if (LittleFS.exists(path)) {
            LittleFS.remove(path);
          }
          uploadFile = LittleFS.open(path, "w");
        }

        if (!uploadFile) {
          return;
        }

        uploadTotalBytes += len;
        if (uploadTotalBytes > MAX_GIF_BYTES) {
          uploadFile.close();
          const String path = presetStore->gifPathForSlot(uploadPresetId);
          LittleFS.remove(path);
          return;
        }

        uploadFile.write(data, len);
        if (final) {
          uploadFile.close();
        }
      });

  server.begin();
  Serial.println("Web server started on port 80");
}
