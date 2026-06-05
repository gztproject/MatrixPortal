#include "display_engine.h"

#include "panel_profile.h"

#include <Preferences.h>
#include <cstring>

namespace {
constexpr int kTextY = 18;
constexpr int kTextSize = 2;
constexpr int kTextBandTop = 4;
constexpr int kTextBandHeight = 20;
constexpr uint8_t kDefaultBrightness = 10;

void setDefaults(SignConfig &cfg) {
  strncpy(cfg.text, "MatrixPortal", sizeof(cfg.text) - 1);
  cfg.text[sizeof(cfg.text) - 1] = '\0';
  cfg.scroll = true;
  cfg.scrollDelayMs = 40;
  cfg.brightness = kDefaultBrightness;
  cfg.colorR = 255;
  cfg.colorG = 255;
  cfg.colorB = 255;
}
}  // namespace

void DisplayEngine::begin(VirtualMatrixPanel *panel, MatrixPanel_I2S_DMA *dma) {
  panel_ = panel;
  dma_ = dma;
  loadFromNvs();
  applyBrightness();
  scrollOffset_ = PANEL_RES_X;
  dirty_ = true;
}

void DisplayEngine::loadFromNvs() {
  setDefaults(config_);

  Preferences prefs;
  if (!prefs.begin("sign", true)) {
    return;
  }

  String text = prefs.getString("text", config_.text);
  text.toCharArray(config_.text, sizeof(config_.text));
  config_.scroll = prefs.getBool("scroll", config_.scroll);
  config_.scrollDelayMs = prefs.getUShort("scrollMs", config_.scrollDelayMs);
  config_.brightness = prefs.getUChar("bright", config_.brightness);
  config_.colorR = prefs.getUChar("colorR", config_.colorR);
  config_.colorG = prefs.getUChar("colorG", config_.colorG);
  config_.colorB = prefs.getUChar("colorB", config_.colorB);
  prefs.end();
}

void DisplayEngine::saveToNvs() {
  Preferences prefs;
  if (!prefs.begin("sign", false)) {
    return;
  }

  prefs.putString("text", config_.text);
  prefs.putBool("scroll", config_.scroll);
  prefs.putUShort("scrollMs", config_.scrollDelayMs);
  prefs.putUChar("bright", config_.brightness);
  prefs.putUChar("colorR", config_.colorR);
  prefs.putUChar("colorG", config_.colorG);
  prefs.putUChar("colorB", config_.colorB);
  prefs.end();
}

SignConfig DisplayEngine::getConfig() const {
  return config_;
}

void DisplayEngine::applyConfig(const SignConfig &cfg, bool persist) {
  config_ = cfg;
  if (config_.text[0] == '\0') {
    setDefaults(config_);
  }
  if (config_.scrollDelayMs < 10) {
    config_.scrollDelayMs = 10;
  }
  if (config_.brightness > 100) {
    config_.brightness = 100;
  }

  applyBrightness();
  scrollOffset_ = PANEL_RES_X;
  dirty_ = true;

  if (persist) {
    saveToNvs();
  }
}

void DisplayEngine::applyBrightness() {
  if (!dma_) {
    return;
  }
  const uint8_t level = (255U * config_.brightness) / 100U;
  dma_->setBrightness8(level);
}

int DisplayEngine::textPixelWidth() const {
  return static_cast<int>(strlen(config_.text)) * 6 * kTextSize;
}

uint16_t DisplayEngine::textColor565() const {
  return panel_->color565(config_.colorR, config_.colorG, config_.colorB);
}

void DisplayEngine::redrawTextBand() {
  if (!panel_) {
    return;
  }

  panel_->fillRect(0, kTextBandTop, PANEL_RES_X, kTextBandHeight, 0);
  panel_->setTextSize(kTextSize);
  panel_->setTextWrap(false);
  panel_->setTextColor(textColor565());

  if (config_.scroll) {
    panel_->setCursor(scrollOffset_, kTextY);
  } else {
    panel_->setCursor(4, kTextY);
  }
  panel_->print(config_.text);
}

void DisplayEngine::tick() {
  if (!panel_) {
    return;
  }

  const unsigned long now = millis();

  if (config_.scroll) {
    if (now - lastScrollMs_ >= config_.scrollDelayMs) {
      lastScrollMs_ = now;
      scrollOffset_--;
      const int textWidth = textPixelWidth();
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
