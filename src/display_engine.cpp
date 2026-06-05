#include "display_engine.h"

#include "gif_player.h"
#include "panel_profile.h"

namespace {
constexpr int kTextY = 18;
constexpr int kTextSize = 2;
constexpr int kTextBandTop = 4;
constexpr int kTextBandHeight = 20;
constexpr unsigned long kFlashMs = 400;
}  // namespace

void DisplayEngine::begin(VirtualMatrixPanel *panel, MatrixPanel_I2S_DMA *dma, PresetStore *store) {
  panel_ = panel;
  dma_ = dma;
  store_ = store;
  gifPlayerBegin(panel_);
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
  store_->set(index, preset);
  if (index == activeIndex()) {
    applyActivePreset();
  }
}

void DisplayEngine::applyActivePreset() {
  runtime_ = store_->get(activeIndex());
  if (runtime_.scrollDelayMs < 10) {
    runtime_.scrollDelayMs = 10;
  }
  if (runtime_.brightness > 100) {
    runtime_.brightness = 100;
  }

  applyBrightness(runtime_.brightness);
  gifPlayerClose();
  scrollOffset_ = PANEL_RES_X;
  dirty_ = true;

  const String slotPath = store_->gifPathForSlot(activeIndex());
  slotPath.toCharArray(runtime_.gifPath, sizeof(runtime_.gifPath));

  gifMode_ = shouldPlayGif(runtime_);
  if (gifMode_) {
    if (!gifPlayerOpen(runtime_.gifPath)) {
      gifMode_ = false;
    }
  }
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

int DisplayEngine::textPixelWidth(const SignPreset &preset) const {
  return static_cast<int>(strlen(preset.text)) * 6 * kTextSize;
}

uint16_t DisplayEngine::textColor565(const SignPreset &preset) const {
  return panel_->color565(preset.colorR, preset.colorG, preset.colorB);
}

void DisplayEngine::redrawTextBand() {
  if (!panel_) {
    return;
  }

  panel_->fillRect(0, kTextBandTop, PANEL_RES_X, kTextBandHeight, 0);
  panel_->setTextSize(kTextSize);
  panel_->setTextWrap(false);
  panel_->setTextColor(textColor565(runtime_));

  if (runtime_.scroll) {
    panel_->setCursor(scrollOffset_, kTextY);
  } else {
    panel_->setCursor(4, kTextY);
  }
  panel_->print(runtime_.text);
}

void DisplayEngine::showPresetFlash(int index) {
  if (!panel_) {
    return;
  }
  flashIndex_ = index;
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
      const int textWidth = textPixelWidth(preset);
      if (scrollOffset_ < -textWidth) {
        scrollOffset_ = PANEL_RES_X;
      }
      dirty_ = true;
    }
  } else if (dirty_) {
    scrollOffset_ = 4;
  }

  if (dirty_) {
    redrawTextBand();
    dirty_ = false;
  }
}

void DisplayEngine::tickGif(const SignPreset &preset) {
  (void)preset;
  gifPlayerTick();
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
    flashIndex_ = -1;
    applyActivePreset();
  }

  if (gifMode_) {
    tickGif(runtime_);
  } else {
    tickText(runtime_);
  }
}
