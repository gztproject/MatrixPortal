#pragma once

#include <Arduino.h>

enum class OtaState : uint8_t { Idle, Checking, Downloading, Flashing, Error };

struct OtaStatus {
  OtaState state = OtaState::Idle;
  uint8_t progress = 0;
  char lastError[64]{};
  char remoteVersion[16]{};
  char remoteBinUrl[160]{};
  bool updateAvailable = false;
};

void otaUpdateBegin();
const char *otaUpdateVersion();
const char *otaUpdateUrl();
bool otaUpdateSetUrl(const char *url);
OtaStatus otaUpdateStatus();
bool otaUpdateIsActive();
bool otaUpdateCheckRemote();
bool otaUpdateStartUpgrade(const char *urlOverride = nullptr);
bool otaUpdateWriteChunk(const uint8_t *data, size_t len, size_t index, size_t total, bool final);
void otaUpdateAbortUpload();
const char *otaUpdateStateString(OtaState state);
