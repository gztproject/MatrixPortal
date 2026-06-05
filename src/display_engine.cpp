#include "display_engine.h"

#include "effect_renderer.h"
#include "gif_player.h"
#include "panel_profile.h"

#include <cstring>

namespace {
constexpr unsigned long kFlashMs = 400;

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}
}  // namespace

void DisplayEngine::begin(VirtualMatrixPanel *panel, MatrixPanel_I2S_DMA *dma, PresetStore *store) {
  panel_ = panel;
  dma_ = dma;
  store_ = store;
  gifPlayerBegin(panel_);
  effectRendererBegin(panel_);
  selectPreset(store_->activeIndex());
}

int DisplayEngine::activeIndex() const {
  return store_ ? store_->activeIndex() : 0;
}

SignPreset DisplayEngine::activePreset() const {
  return runtime_;
}

void DisplayEngine::selectPreset(int index) {
  if (!store_) {
    return;
  }
  store_->setActiveIndex(index);
  applyActivePreset();
  showPresetFlash(index);
}

void DisplayEngine::applyPreset(const SignPreset &preset, int index, bool persist) {
  if (!store_ || index < 0 || index >= PRESET_COUNT) {
    return;
  }

  SignPreset normalized = preset;
  if (normalized.rowCount < 1) {
    normalized.rowCount = 1;
  }
  if (normalized.rowCount > 4) {
    normalized.rowCount = 4;
  }
  normalized.textHeightPx = clampTextHeightPx(normalized.textHeightPx);

  store_->set(index, normalized);
  if (index == activeIndex()) {
    applyActivePreset();
  }
}

void DisplayEngine::resolveActiveContentType() {
  if (runtime_.contentType == ContentType::Effect &&
      static_cast<EffectId>(runtime_.effectId) != EffectId::None) {
    activeContentType_ = ContentType::Effect;
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
  if (runtime_.scrollDelayMs < 10) {
    runtime_.scrollDelayMs = 10;
  }
  if (runtime_.brightness > 100) {
    runtime_.brightness = 100;
  }
  if (runtime_.rowCount < 1) {
    runtime_.rowCount = 1;
  }
  if (runtime_.rowCount > 4) {
    runtime_.rowCount = 4;
  }

  applyBrightness(runtime_.brightness);
  gifPlayerClose();
  scrollOffset_ = PANEL_RES_X;
  dirty_ = true;

  const String slotPath = store_->gifPathForSlot(activeIndex());
  slotPath.toCharArray(runtime_.gifPath, sizeof(runtime_.gifPath));

  resolveActiveContentType();

  panel_->fillScreen(0);

  if (activeContentType_ == ContentType::Effect) {
    effectRendererApply(static_cast<EffectId>(runtime_.effectId));
    return;
  }

  if (activeContentType_ == ContentType::Gif) {
    if (!gifPlayerOpen(runtime_.gifPath)) {
      activeContentType_ = ContentType::Text;
    } else {
      return;
    }
  }

  textLayout_ = computeTextLayout(runtime_);
  textLineCount_ = splitTextLines(runtime_, textLines_, runtime_.rowCount);
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

DisplayEngine::TextLayout DisplayEngine::computeTextLayout(const SignPreset &preset) const {
  TextLayout layout;
  layout.rowCount = clampInt(preset.rowCount, 1, 4);
  layout.blockHeight = clampTextHeightPx(preset.textHeightPx);
  layout.blockTop = (PANEL_RES_Y - layout.blockHeight) / 2;
  const int lineHeight = layout.blockHeight / layout.rowCount;
  layout.textSize = clampInt(lineHeight / 8, 1, 6);

  const int glyphHeight = 8 * layout.textSize;
  for (int i = 0; i < layout.rowCount; i++) {
    layout.rowY[i] = layout.blockTop + i * lineHeight + (lineHeight - glyphHeight) / 2;
  }
  return layout;
}

int DisplayEngine::splitTextLines(const SignPreset &preset, char lines[][201], int maxLines) const {
  int count = 0;
  const char *cursor = preset.text;

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
    const int width = static_cast<int>(strlen(lines[i])) * 6 * layout.textSize;
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
  panel_->setTextSize(textLayout_.textSize);
  panel_->setTextWrap(false);
  panel_->setTextColor(textColor565(runtime_));

  const int x = runtime_.scroll ? scrollOffset_ : 4;
  for (int i = 0; i < textLineCount_; i++) {
    panel_->setCursor(x, textLayout_.rowY[i]);
    panel_->print(textLines_[i]);
  }
}

void DisplayEngine::showPresetFlash(int index) {
  if (!panel_) {
    return;
  }
  flashUntilMs_ = millis() + kFlashMs;
  panel_->fillScreen(0);
  panel_->setTextSize(1);
  panel_->setTextWrap(false);
  panel_->setTextColor(panel_->color565(180, 180, 180));
  panel_->setCursor(36, 24);
  panel_->print(index + 1);
  panel_->print('/');
  panel_->print(PRESET_COUNT);
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
  } else if (dirty_) {
    scrollOffset_ = 4;
  }

  if (dirty_) {
    redrawTextBlock();
    dirty_ = false;
  }
}

void DisplayEngine::tickGif(const SignPreset &preset) {
  (void)preset;
  gifPlayerTick();
}

void DisplayEngine::tickEffect(const SignPreset &preset) {
  effectRendererTick(static_cast<EffectId>(preset.effectId));
}

void DisplayEngine::tick() {
  if (!panel_ || !store_) {
    return;
  }

  if (flashUntilMs_ > 0 && millis() < flashUntilMs_) {
    return;
  }

  if (flashUntilMs_ > 0 && millis() >= flashUntilMs_) {
    flashUntilMs_ = 0;
    applyActivePreset();
  }

  switch (activeContentType_) {
    case ContentType::Effect:
      tickEffect(runtime_);
      break;
    case ContentType::Gif:
      tickGif(runtime_);
      break;
    case ContentType::Text:
    default:
      tickText(runtime_);
      break;
  }
}
