#include "time_sync.h"

#include "wifi_manager.h"

#include <sys/time.h>
#include <time.h>

namespace {
constexpr unsigned long kResyncMs = 3600000UL;
constexpr long kMinValidUnix = 1700000000L;

struct TimezoneOption {
  const char *id;
  const char *posix;
};

constexpr TimezoneOption kTimezones[] = {
    {"CET", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"UTC", "UTC0"},
    {"WET", "WET0WEST,M3.5.0/1,M10.5.0"},
    {"EET", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"GMT", "GMT0BST,M3.5.0/1,M10.5.0"},
};

bool timeValid = false;
TimeSyncSource timeSource = TimeSyncSource::None;
unsigned long lastSyncAttemptMs = 0;
unsigned long lastSyncSuccessMs = 0;

const TimezoneOption *findTimezone(const char *timezoneId) {
  if (!timezoneId || timezoneId[0] == '\0') {
    return nullptr;
  }
  for (const TimezoneOption &option : kTimezones) {
    if (strcmp(option.id, timezoneId) == 0) {
      return &option;
    }
  }
  return nullptr;
}

void applyPosixTimezone(const char *posixTz) {
  if (!posixTz || posixTz[0] == '\0') {
    return;
  }
  setenv("TZ", posixTz, 1);
  tzset();
}
}  // namespace

void timeSyncBegin() {
  applyPosixTimezone(kTimezones[0].posix);
}

void timeSyncApplyTimezone(const char *timezoneId) {
  const TimezoneOption *option = findTimezone(timeSyncNormalizeTimezoneId(timezoneId));
  if (!option) {
    return;
  }
  applyPosixTimezone(option->posix);
}

const char *timeSyncDefaultTimezoneId() {
  return kTimezones[0].id;
}

const char *timeSyncNormalizeTimezoneId(const char *timezoneId) {
  if (findTimezone(timezoneId) != nullptr) {
    return timezoneId;
  }
  return timeSyncDefaultTimezoneId();
}

bool timeSyncIsKnownTimezoneId(const char *timezoneId) {
  return findTimezone(timezoneId) != nullptr;
}

void timeSyncTick() {
  if (!wifiStaConnected()) {
    return;
  }

  const unsigned long now = millis();
  if (timeValid && now - lastSyncSuccessMs < kResyncMs) {
    return;
  }
  if (now - lastSyncAttemptMs < 30000UL) {
    return;
  }

  lastSyncAttemptMs = now;
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  for (int i = 0; i < 20; i++) {
    delay(100);
    const time_t t = time(nullptr);
    if (t >= kMinValidUnix) {
      timeValid = true;
      timeSource = TimeSyncSource::Ntp;
      lastSyncSuccessMs = millis();
      return;
    }
  }
}

bool timeIsValid() {
  if (!timeValid) {
    return false;
  }
  return time(nullptr) >= kMinValidUnix;
}

TimeSyncSource timeSyncSource() {
  if (!timeIsValid()) {
    return TimeSyncSource::None;
  }
  return timeSource;
}

const char *timeSyncSourceString(TimeSyncSource source) {
  switch (source) {
    case TimeSyncSource::Ntp:
      return "ntp";
    case TimeSyncSource::Browser:
      return "browser";
    default:
      return "none";
  }
}

bool timeSyncSetUnix(time_t unixSeconds) {
  if (unixSeconds < kMinValidUnix) {
    return false;
  }

  struct timeval tv {};
  tv.tv_sec = unixSeconds;
  tv.tv_usec = 0;
  if (settimeofday(&tv, nullptr) != 0) {
    return false;
  }

  timeValid = true;
  timeSource = TimeSyncSource::Browser;
  lastSyncSuccessMs = millis();
  return true;
}

time_t timeNowUnix() {
  return time(nullptr);
}

void formatClockTime(char *buf, size_t len, bool showSeconds) {
  if (!buf || len == 0) {
    return;
  }
  if (!timeIsValid()) {
    snprintf(buf, len, "--:--");
    return;
  }

  struct tm localTime {};
  const time_t now = time(nullptr);
  localtime_r(&now, &localTime);
  if (showSeconds) {
    snprintf(buf, len, "%02d:%02d:%02d", localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
  } else {
    snprintf(buf, len, "%02d:%02d", localTime.tm_hour, localTime.tm_min);
  }
}

void formatClockDate(char *buf, size_t len) {
  if (!buf || len == 0) {
    return;
  }
  if (!timeIsValid()) {
    snprintf(buf, len, "--.--.----");
    return;
  }

  struct tm localTime {};
  const time_t now = time(nullptr);
  localtime_r(&now, &localTime);
  snprintf(buf, len, "%02d.%02d.%04d", localTime.tm_mday, localTime.tm_mon + 1,
           localTime.tm_year + 1900);
}

void formatCountdownSeconds(char *buf, size_t len, long remainingSec) {
  if (!buf || len == 0) {
    return;
  }
  if (remainingSec <= 0) {
    snprintf(buf, len, "0:00");
    return;
  }

  const long hours = remainingSec / 3600;
  remainingSec %= 3600;
  const long minutes = remainingSec / 60;
  const long seconds = remainingSec % 60;
  if (hours > 0) {
    snprintf(buf, len, "%ld:%02ld:%02ld", hours, minutes, seconds);
  } else {
    snprintf(buf, len, "%ld:%02ld", minutes, seconds);
  }
}

void formatCountdown(char *buf, size_t len, time_t targetUnix) {
  if (!buf || len == 0) {
    return;
  }
  if (!timeIsValid() || targetUnix == 0) {
    snprintf(buf, len, "--:--");
    return;
  }

  const time_t now = timeNowUnix();
  const long remaining = static_cast<long>(targetUnix - now);
  formatCountdownSeconds(buf, len, remaining);
}
