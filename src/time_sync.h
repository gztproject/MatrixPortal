#pragma once

#include <Arduino.h>

enum class TimeSyncSource : uint8_t { None, Browser, Ntp };

void timeSyncBegin();
void timeSyncTick();
bool timeIsValid();
time_t timeNowUnix();
TimeSyncSource timeSyncSource();
const char *timeSyncSourceString(TimeSyncSource source);
bool timeSyncSetUnix(time_t unixSeconds);
void timeSyncApplyTimezone(const char *timezoneId);
const char *timeSyncDefaultTimezoneId();
const char *timeSyncNormalizeTimezoneId(const char *timezoneId);
bool timeSyncIsKnownTimezoneId(const char *timezoneId);
void formatClockTime(char *buf, size_t len, bool showSeconds);
void formatClockDate(char *buf, size_t len);
void formatCountdown(char *buf, size_t len, time_t targetUnix);
void formatCountdownSeconds(char *buf, size_t len, long remainingSec);
