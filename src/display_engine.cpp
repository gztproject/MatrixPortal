#include "display_engine.h"

#include "effect_renderer.h"
#include "gif_player.h"
#include "panel_profile.h"
#include "text_renderer.h"
#include "time_sync.h"
#include "ota_update.h"

#include <cstring>
#include <freertos/semphr.h>

namespace {
SemaphoreHandle_t engineMutex = nullptr;

class DisplayLock {
 public:
  DisplayLock() {
    if (engineMutex != nullptr) {
      xSemaphoreTakeRecursive(engineMutex, portMAX_DELAY);
    }
  }
  ~DisplayLock() {
    if (engineMutex != nullptr) {
      xSemaphoreGiveRecursive(engineMutex);
    }
  }
};

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

SignPreset normalizePreset(SignPreset preset) {
  if (preset.rowCount < 1) {
    preset.rowCount = 1;
  }
  if (preset.rowCount > 4) {
    preset.rowCount = 4;
  }
  preset.textHeightPx = clampTextHeightPx(preset.textHeightPx);
  if (preset.scrollDelayMs < 10) {
    preset.scrollDelayMs = 10;
  }
  preset.contentOffsetX = clampContentOffset(preset.contentOffsetX);
  preset.contentOffsetY = clampContentOffset(preset.contentOffsetY);
  return preset;
}
int fitTimeTextSize(const char *text) {
  const int unitWidth = textLinePixelWidth(text, 1);
  if (unitWidth <= 0) {
    return 1;
  }
  return clampInt(PANEL_RES_X / unitWidth, 1, 6);
}

constexpr uint8_t kClockFlagSeconds = 1;
constexpr uint8_t kClockFlagDate = 2;

bool clockShowSeconds(uint8_t effectParam) {
  return (effectParam & kClockFlagSeconds) != 0 || effectParam > 3;
}

bool clockShowDate(uint8_t effectParam) {
  return (effectParam & kClockFlagDate) != 0;
}

void layoutClockLines(const char *timeStr, const char *dateStr, bool showDate, int &timeSize,
                      int &dateSize, int &timeY, int &dateY) {
  timeSize = fitTimeTextSize(timeStr);
  if (!showDate || dateStr[0] == '\0') {
    dateSize = 0;
    timeY = (PANEL_RES_Y - kTextBodyBandRows * timeSize) / 2;
    dateY = 0;
    return;
  }

  dateSize = fitTimeTextSize(dateStr);
  int gap = timeSize;
  int totalH = kTextBodyBandRows * timeSize + gap + kTextBodyBandRows * dateSize;
  while (totalH > PANEL_RES_Y && (timeSize > 1 || dateSize > 1)) {
    if (timeSize >= dateSize && timeSize > 1) {
      timeSize--;
    } else if (dateSize > 1) {
      dateSize--;
    } else {
      break;
    }
    gap = timeSize;
    totalH = kTextBodyBandRows * timeSize + gap + kTextBodyBandRows * dateSize;
  }

  const int top = (PANEL_RES_Y - totalH) / 2;
  timeY = top;
  dateY = top + kTextBodyBandRows * timeSize + gap;
}
}  // namespace

void DisplayEngine::begin(VirtualMatrixPanel *panel, MatrixPanel_I2S_DMA *dma, PresetStore *store) {
  if (engineMutex == nullptr) {
    engineMutex = xSemaphoreCreateRecursiveMutex();
  }
  DisplayLock lock;
  panel_ = panel;
  dma_ = dma;
  store_ = store;
  gifPlayerBegin(panel_);
  effectRendererBegin(panel_);
  playlistSlotSinceMs_ = millis();
  selectPreset(store_->activeIndex());
  if (!store_->displayOn()) {
    showDisplayOffIndicator();
  }
}

int DisplayEngine::activeIndex() const {
  DisplayLock lock;
  return store_ ? store_->activeIndex() : 0;
}

SignPreset DisplayEngine::activePreset() const {
  DisplayLock lock;
  return runtime_;
}

uint8_t DisplayEngine::globalBrightness() const {
  DisplayLock lock;
  return store_ ? store_->globalBrightness() : GLOBAL_BRIGHTNESS_DEFAULT;
}

bool DisplayEngine::displayOn() const {
  DisplayLock lock;
  return store_ ? store_->displayOn() : true;
}

void DisplayEngine::setDisplayOn(bool on, bool persist) {
  DisplayLock lock;
  if (!store_) {
    return;
  }
  store_->setDisplayOn(on, persist);
  applyDisplayPowerState();
}

void DisplayEngine::toggleDisplayOn() {
  DisplayLock lock;
  if (!store_) {
    return;
  }
  setDisplayOn(!store_->displayOn());
}

void DisplayEngine::setGlobalBrightness(uint8_t percent) {
  DisplayLock lock;
  if (!store_) {
    return;
  }
  store_->setGlobalBrightness(percent);
  if (store_->displayOn()) {
    applyBrightness(store_->globalBrightness());
  }
}

void DisplayEngine::adjustGlobalBrightness(int delta) {
  DisplayLock lock;
  if (!store_ || delta == 0 || !store_->displayOn()) {
    return;
  }
  const int current = static_cast<int>(store_->globalBrightness());
  if (delta > 0 && current >= 100) {
    return;
  }
  if (delta < 0 && current <= 1) {
    return;
  }
  int next = current + delta;
  if (next < 1) {
    next = 1;
  } else if (next > 100) {
    next = 100;
  }
  setGlobalBrightness(static_cast<uint8_t>(next));
}

void DisplayEngine::selectPreset(int index) {
  DisplayLock lock;
  if (!store_) {
    return;
  }
  store_->setActiveIndex(index);
  playlistSlotSinceMs_ = millis();
  applyActivePreset();
}

bool DisplayEngine::applyPreset(const SignPreset &preset, int index, bool persist) {
  DisplayLock lock;
  if (!store_ || index < 0 || index >= PRESET_COUNT) {
    return false;
  }

  const SignPreset normalized = normalizePreset(preset);

  if (persist) {
    if (!store_->set(index, normalized)) {
      return false;
    }
  }
  if (index == activeIndex()) {
    runtime_ = normalized;
    applyRuntimePreset(index);
  }
  return true;
}

void DisplayEngine::previewOnPanel(const SignPreset &preset, int gifSlotIndex) {
  DisplayLock lock;
  if (!store_ || gifSlotIndex < 0 || gifSlotIndex >= PRESET_COUNT) {
    return;
  }

  runtime_ = normalizePreset(preset);
  applyRuntimePreset(gifSlotIndex);
}

void DisplayEngine::resolveActiveContentType() {
  if (runtime_.contentType == ContentType::Effect &&
      static_cast<EffectId>(runtime_.effectId) != EffectId::None) {
    activeContentType_ = ContentType::Effect;
    return;
  }
  if (runtime_.contentType == ContentType::Clock) {
    activeContentType_ = ContentType::Clock;
    return;
  }
  if (runtime_.contentType == ContentType::Countdown) {
    activeContentType_ = ContentType::Countdown;
    return;
  }
  if (runtime_.contentType == ContentType::Gif && shouldPlayGif(runtime_)) {
    activeContentType_ = ContentType::Gif;
    return;
  }
  activeContentType_ = ContentType::Text;
}

void DisplayEngine::applyActivePreset() {
  runtime_ = store_->get(activeIndex());
  applyRuntimePreset(activeIndex());
}

void DisplayEngine::applyRuntimePreset(int gifSlotIndex) {
  if (runtime_.scrollDelayMs < 10) {
    runtime_.scrollDelayMs = 10;
  }
  if (runtime_.rowCount < 1) {
    runtime_.rowCount = 1;
  }
  if (runtime_.rowCount > 4) {
    runtime_.rowCount = 4;
  }

  if (store_ && !store_->displayOn()) {
    showDisplayOffIndicator();
    return;
  }

  applyBrightness(store_->globalBrightness());
  gifPlayerClose();
  scrollOffset_ = PANEL_RES_X;
  dirty_ = true;

  const String slotPath = store_->gifPathForSlot(gifSlotIndex);
  slotPath.toCharArray(runtime_.gifPath, sizeof(runtime_.gifPath));

  resolveActiveContentType();

  clearAllBuffers();

  if (activeContentType_ == ContentType::Effect) {
    effectRendererApply(static_cast<EffectId>(runtime_.effectId), runtime_.colorR, runtime_.colorG,
                        runtime_.colorB, runtime_.effectParam);
    finishFrame();
    return;
  }

  if (activeContentType_ == ContentType::Clock || activeContentType_ == ContentType::Countdown) {
    countdownStartedMs_ = millis();
    dirty_ = true;
    lastScrollMs_ = millis();
    redrawTimeBlock();
    return;
  }

  if (activeContentType_ == ContentType::Gif) {
    if (!gifPlayerOpen(runtime_.gifPath)) {
      activeContentType_ = ContentType::Text;
    } else {
      return;
    }
  }

  textLineCount_ = splitTextLines(runtime_, textLines_, runtime_.rowCount);
  textLayout_ = computeTextLayout(runtime_, textLines_, textLineCount_);
  redrawTextBlock();
}

bool DisplayEngine::shouldPlayGif(const SignPreset &preset) const {
  if (preset.gifPath[0] == '\0') {
    return false;
  }
  return gifPlayerFileExists(preset.gifPath);
}

void DisplayEngine::applyBrightness(uint8_t brightnessPercent) {
  if (!dma_) {
    return;
  }
  const uint8_t level = (255U * brightnessPercent) / 100U;
  dma_->setBrightness8(level);
}

void DisplayEngine::clearAllBuffers() {
  if (!panel_ || !dma_) {
    return;
  }
  panel_->fillScreen(0);
  if (dma_->getCfg().double_buff) {
    dma_->flipDMABuffer();
    panel_->fillScreen(0);
    dma_->flipDMABuffer();
  }
}

void DisplayEngine::finishFrame() {
  if (dma_) {
    dma_->flipDMABuffer();
  }
}

void DisplayEngine::showDisplayOffIndicator() {
  if (!panel_ || !dma_) {
    return;
  }
  gifPlayerClose();
  applyBrightness(DISPLAY_OFF_BRIGHTNESS);
  clearAllBuffers();
  panel_->drawPixel(0, 0, panel_->color565(255, 0, 0));
  dirty_ = false;
  finishFrame();
}

void DisplayEngine::applyDisplayPowerState() {
  if (!store_) {
    return;
  }
  if (!store_->displayOn()) {
    showDisplayOffIndicator();
    return;
  }
  applyActivePreset();
}

DisplayEngine::TextLayout DisplayEngine::computeTextLayout(const SignPreset &preset,
                                                           char lines[][201],
                                                           int lineCount) const {
  TextLayout layout;
  layout.rowCount = clampInt(preset.rowCount, 1, 4);
  layout.blockHeight = clampTextHeightPx(preset.textHeightPx);
  layout.blockTop = (PANEL_RES_Y - layout.blockHeight) / 2;
  const int lineHeight = layout.blockHeight / layout.rowCount;
  layout.textSize = clampInt(lineHeight / kTextBodyBandRows, 1, 6);

  const int caronHeight = kTextCaronBandRows * layout.textSize;
  const int bodyHeight = kTextBodyBandRows * layout.textSize;
  const int rows = clampInt(lineCount, 1, layout.rowCount);

  bool rowHasCaron[4] = {};
  for (int i = 0; i < rows; i++) {
    rowHasCaron[i] = textLineHasCaron(lines[i]);
  }

  int totalHeight = 0;
  for (int i = 0; i < rows; i++) {
    if (i > 0 && rowHasCaron[i]) {
      totalHeight += caronHeight;
    }
    totalHeight += rowHasCaron[i] ? (caronHeight + bodyHeight) : bodyHeight;
  }

  int y = layout.blockTop + (layout.blockHeight - totalHeight) / 2;
  if (y < layout.blockTop) {
    y = layout.blockTop;
  }

  for (int i = 0; i < layout.rowCount; i++) {
    if (i >= rows) {
      layout.rowY[i] = y;
      continue;
    }
    if (i > 0 && rowHasCaron[i]) {
      y += caronHeight;
    }
    if (rowHasCaron[i]) {
      layout.rowY[i] = y + caronHeight;
      y += caronHeight + bodyHeight;
    } else {
      layout.rowY[i] = y;
      y += bodyHeight;
    }
  }
  return layout;
}

int DisplayEngine::splitTextLines(const SignPreset &preset, char lines[][201], int maxLines) const {
  int count = 0;
  const char *cursor = preset.message;

  while (*cursor && count < maxLines) {
    const char *next = strchr(cursor, '\n');
    const size_t len = next ? static_cast<size_t>(next - cursor) : strlen(cursor);
    const size_t copyLen = len < 200 ? len : 200;
    memcpy(lines[count], cursor, copyLen);
    lines[count][copyLen] = '\0';
    count++;
    if (!next) {
      break;
    }
    cursor = next + 1;
  }

  if (count == 0) {
    lines[0][0] = '\0';
    count = 1;
  }
  return count;
}

int DisplayEngine::textBlockPixelWidth(const SignPreset &preset, const TextLayout &layout,
                                       char lines[][201], int lineCount) const {
  int maxWidth = 0;
  for (int i = 0; i < lineCount; i++) {
    const int width = textLinePixelWidth(lines[i], layout.textSize);
    if (width > maxWidth) {
      maxWidth = width;
    }
  }
  (void)preset;
  return maxWidth;
}

uint16_t DisplayEngine::textColor565(const SignPreset &preset) const {
  return panel_->color565(preset.colorR, preset.colorG, preset.colorB);
}

void DisplayEngine::redrawTextBlock() {
  if (!panel_) {
    return;
  }

  panel_->fillRect(0, textLayout_.blockTop, PANEL_RES_X, textLayout_.blockHeight, 0);
  const uint16_t color = textColor565(runtime_);

  for (int i = 0; i < textLineCount_; i++) {
    int x = scrollOffset_;
    if (!runtime_.scroll) {
      const int width = textLinePixelWidth(textLines_[i], textLayout_.textSize);
      x = (PANEL_RES_X - width) / 2;
    }
    x += runtime_.contentOffsetX;
    const int y = textLayout_.rowY[i] + runtime_.contentOffsetY;
    drawTextLine(panel_, x, y, textLayout_.textSize, color, textLines_[i]);
  }
  finishFrame();
}

void DisplayEngine::redrawTimeBlock() {
  if (!panel_) {
    return;
  }

  if (activeContentType_ == ContentType::Clock) {
    const bool showSeconds = clockShowSeconds(runtime_.effectParam);
    const bool showDate = clockShowDate(runtime_.effectParam);
    formatClockTime(timeLine_, sizeof(timeLine_), showSeconds);
    if (showDate) {
      formatClockDate(dateLine_, sizeof(dateLine_));
    } else {
      dateLine_[0] = '\0';
    }

    int timeSize = 1;
    int dateSize = 0;
    int timeY = 0;
    int dateY = 0;
    layoutClockLines(timeLine_, dateLine_, showDate, timeSize, dateSize, timeY, dateY);

    int clearTop = timeY - kTextCaronBandRows * timeSize;
    int clearBottom = timeY + kTextBodyBandRows * timeSize;
    if (showDate) {
      const int dateTop = dateY - kTextCaronBandRows * dateSize;
      const int dateBottom = dateY + kTextBodyBandRows * dateSize;
      if (dateTop < clearTop) {
        clearTop = dateTop;
      }
      if (dateBottom > clearBottom) {
        clearBottom = dateBottom;
      }
    }
    if (clearTop < 0) {
      clearTop = 0;
    }
    if (clearBottom > PANEL_RES_Y) {
      clearBottom = PANEL_RES_Y;
    }
    panel_->fillRect(0, clearTop, PANEL_RES_X, clearBottom - clearTop, 0);

    const uint16_t color = textColor565(runtime_);
    const int timeX = (PANEL_RES_X - textLinePixelWidth(timeLine_, timeSize)) / 2;
    drawTextLine(panel_, timeX, timeY, timeSize, color, timeLine_);
    if (showDate) {
      const int dateX = (PANEL_RES_X - textLinePixelWidth(dateLine_, dateSize)) / 2;
      drawTextLine(panel_, dateX, dateY, dateSize, color, dateLine_);
    }
    finishFrame();
    return;
  }

  if (runtime_.countdownDurationSec > 0) {
    const unsigned long elapsedMs = millis() - countdownStartedMs_;
    const long remaining =
        static_cast<long>(runtime_.countdownDurationSec) - static_cast<long>(elapsedMs / 1000);
    formatCountdownSeconds(timeLine_, sizeof(timeLine_), remaining);
  } else {
    formatCountdown(timeLine_, sizeof(timeLine_), runtime_.countdownEndUnix);
  }

  const int textSize = fitTimeTextSize(timeLine_);
  const int width = textLinePixelWidth(timeLine_, textSize);
  const int x = (PANEL_RES_X - width) / 2;
  const int y = (PANEL_RES_Y - kTextBodyBandRows * textSize) / 2;
  const int clearTop = y - kTextCaronBandRows * textSize;
  const int clearHeight = kTextCaronBandRows * textSize + kTextBodyBandRows * textSize;
  panel_->fillRect(0, clearTop < 0 ? 0 : clearTop, PANEL_RES_X,
                   clearTop < 0 ? clearHeight + clearTop : clearHeight, 0);
  drawTextLine(panel_, x, y, textSize, textColor565(runtime_), timeLine_);
  finishFrame();
}

void DisplayEngine::redrawOtaScreen(uint8_t progressPercent) {
  if (!panel_) {
    return;
  }

  panel_->fillScreen(0);

  static const char kTitle[] = "Updating...";
  char progressLine[8];
  snprintf(progressLine, sizeof(progressLine), "%u%%", progressPercent);

  int titleSize = fitTimeTextSize(kTitle);
  int progressSize = fitTimeTextSize(progressLine);
  int gap = titleSize;
  int totalH = kTextBodyBandRows * titleSize + gap + kTextBodyBandRows * progressSize;
  while (totalH > PANEL_RES_Y && (titleSize > 1 || progressSize > 1)) {
    if (titleSize >= progressSize && titleSize > 1) {
      titleSize--;
    } else if (progressSize > 1) {
      progressSize--;
    } else {
      break;
    }
    gap = titleSize;
    totalH = kTextBodyBandRows * titleSize + gap + kTextBodyBandRows * progressSize;
  }

  const int top = (PANEL_RES_Y - totalH) / 2;
  const uint16_t color = panel_->color565(255, 128, 0);
  const int titleX = (PANEL_RES_X - textLinePixelWidth(kTitle, titleSize)) / 2;
  const int progressX = (PANEL_RES_X - textLinePixelWidth(progressLine, progressSize)) / 2;
  drawTextLine(panel_, titleX, top, titleSize, color, kTitle);
  drawTextLine(panel_, progressX, top + kTextBodyBandRows * titleSize + gap, progressSize, color,
               progressLine);
  finishFrame();
}

void DisplayEngine::tickTime(const SignPreset &preset) {
  (void)preset;
  const unsigned long now = millis();
  if (now - lastScrollMs_ >= 1000) {
    lastScrollMs_ = now;
    if (activeContentType_ == ContentType::Clock) {
      char newTime[32];
      char newDate[16] = "";
      const bool showSeconds = clockShowSeconds(runtime_.effectParam);
      formatClockTime(newTime, sizeof(newTime), showSeconds);
      if (clockShowDate(runtime_.effectParam)) {
        formatClockDate(newDate, sizeof(newDate));
      }
      if (strcmp(newTime, timeLine_) != 0 || strcmp(newDate, dateLine_) != 0) {
        dirty_ = true;
      }
    } else {
      dirty_ = true;
    }
  }
  if (dirty_) {
    redrawTimeBlock();
    dirty_ = false;
  }
}

void DisplayEngine::refreshTimeDisplay() {
  DisplayLock lock;
  if (activeContentType_ == ContentType::Clock || activeContentType_ == ContentType::Countdown) {
    redrawTimeBlock();
  }
}

int DisplayEngine::nextPlaylistSlot(int current) const {
  if (!store_) {
    return current;
  }
  const PlaylistSettings playlist = store_->playlist();
  if (playlist.slotMask == 0) {
    return current;
  }

  for (int step = 1; step <= PRESET_COUNT; step++) {
    const int candidate = (current + step) % PRESET_COUNT;
    if ((playlist.slotMask & (1 << candidate)) != 0) {
      return candidate;
    }
  }
  return current;
}

void DisplayEngine::tickPlaylist() {
  if (!store_) {
    return;
  }

  const PlaylistSettings playlist = store_->playlist();
  if (!playlist.enabled) {
    return;
  }

  int enabledCount = 0;
  for (int i = 0; i < PRESET_COUNT; i++) {
    if (playlist.slotMask & (1 << i)) {
      enabledCount++;
    }
  }
  if (enabledCount <= 1) {
    return;
  }

  if ((playlist.slotMask & (1 << activeIndex())) == 0) {
    selectPreset(nextPlaylistSlot(activeIndex() - 1));
    return;
  }

  const unsigned long now = millis();
  if (now - playlistSlotSinceMs_ < playlist.dwellMs) {
    return;
  }

  selectPreset(nextPlaylistSlot(activeIndex()));
}

void DisplayEngine::tickText(const SignPreset &preset) {
  const unsigned long now = millis();

  if (preset.scroll) {
    if (now - lastScrollMs_ >= preset.scrollDelayMs) {
      lastScrollMs_ = now;
      scrollOffset_--;
      const int textWidth = textBlockPixelWidth(preset, textLayout_, textLines_, textLineCount_);
      if (scrollOffset_ < -textWidth) {
        scrollOffset_ = PANEL_RES_X;
      }
      dirty_ = true;
    }
  }

  if (dirty_) {
    redrawTextBlock();
    dirty_ = false;
  }
}

void DisplayEngine::tickGif(const SignPreset &preset) {
  (void)preset;
  if (gifPlayerTick()) {
    finishFrame();
  }
}

void DisplayEngine::tickEffect(const SignPreset &preset) {
  if (effectRendererTick(static_cast<EffectId>(preset.effectId))) {
    finishFrame();
  }
}

void DisplayEngine::tick() {
  DisplayLock lock;
  if (!panel_ || !store_) {
    return;
  }

  static uint8_t lastOtaProgress = 255;

  if (otaUpdateIsActive()) {
    const OtaStatus ota = otaUpdateStatus();
    if (lastOtaProgress != ota.progress) {
      redrawOtaScreen(ota.progress);
      lastOtaProgress = ota.progress;
    }
    return;
  }

  if (lastOtaProgress != 255) {
    lastOtaProgress = 255;
    dirty_ = true;
    applyActivePreset();
  }

  timeSyncTick();
  tickPlaylist();

  if (!store_->displayOn()) {
    return;
  }

  switch (activeContentType_) {
    case ContentType::Effect:
      tickEffect(runtime_);
      break;
    case ContentType::Gif:
      tickGif(runtime_);
      break;
    case ContentType::Clock:
    case ContentType::Countdown:
      tickTime(runtime_);
      break;
    case ContentType::Text:
    default:
      tickText(runtime_);
      break;
  }
}
