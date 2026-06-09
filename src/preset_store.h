#pragma once

#include <Arduino.h>

#define PRESET_COUNT 8
#define PRESET_LABEL_MAX 16
#define MAX_GIF_BYTES (256 * 1024)
#define TEXT_HEIGHT_PX_MIN 8
#define TEXT_HEIGHT_PX_MAX 52
#define TEXT_HEIGHT_PX_DEFAULT 26
#define GLOBAL_BRIGHTNESS_DEFAULT 10
#define DISPLAY_OFF_BRIGHTNESS 5
#define PLAYLIST_DWELL_MS_DEFAULT 8000
#define PLAYLIST_DWELL_MS_MIN 1000
#define PLAYLIST_DWELL_MS_MAX 3600000
#define TIMEZONE_ID_MAX 8

enum class ContentType : uint8_t { Text, Gif, Effect, Clock, Countdown };

struct SignPreset {
  ContentType contentType;
  uint8_t effectId;
  uint8_t textHeightPx;
  uint8_t rowCount;
  char message[201];
  char label[PRESET_LABEL_MAX + 1];
  bool scroll;
  uint16_t scrollDelayMs;
  uint8_t colorR;
  uint8_t colorG;
  uint8_t colorB;
  uint8_t effectParam;
  uint32_t countdownEndUnix;
  uint32_t countdownDurationSec;
  int8_t contentOffsetX;
  int8_t contentOffsetY;
  char gifPath[48];
};

struct PlaylistSettings {
  bool enabled = false;
  uint8_t slotMask = 0xFF;
  uint32_t dwellMs = PLAYLIST_DWELL_MS_DEFAULT;
};

const char *contentTypeToString(ContentType type);
ContentType contentTypeFromString(const char *value);
uint8_t clampTextHeightPx(int px);
uint8_t textHeightPxFromLegacyFontScale(const char *value);
uint8_t clampBrightness(int value);
uint32_t clampPlaylistDwellMs(int ms);
int8_t clampContentOffset(int px);

class PresetStore {
 public:
  void begin();
  SignPreset get(int index) const;
  bool set(int index, const SignPreset &preset);
  int activeIndex() const;
  void setActiveIndex(int index, bool persist = true);
  String gifPathForSlot(int index) const;
  bool gifExistsForSlot(int index) const;
  bool duplicateSlot(int fromIndex, int toIndex);
  uint8_t globalBrightness() const;
  void setGlobalBrightness(uint8_t value, bool persist = true);
  PlaylistSettings playlist() const;
  void setPlaylist(const PlaylistSettings &settings, bool persist = true);
  const char *timezoneId() const;
  void setTimezoneId(const char *id, bool persist = true);
  bool displayOn() const;
  void setDisplayOn(bool on, bool persist = true);

 private:
  void loadAll();
  bool persistPreset(int index, const SignPreset &preset);
  void saveActiveIndex();
  void saveGlobalBrightness();
  void savePlaylist();
  void saveTimezoneId();
  void saveDisplayOn();
  void migrateLegacySign();
  void setDefaults(SignPreset &preset) const;

  SignPreset presets_[PRESET_COUNT]{};
  int activeIndex_ = 0;
  uint8_t globalBrightness_ = GLOBAL_BRIGHTNESS_DEFAULT;
  PlaylistSettings playlist_{};
  char timezoneId_[TIMEZONE_ID_MAX + 1]{};
  bool displayOn_ = true;
};
