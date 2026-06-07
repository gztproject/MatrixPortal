#include "panel_profile.h"

#include <Arduino.h>

#define PANEL_CHAIN 1
#define PANEL_REFRESH_HZ 90
#define PANEL_COLOR_DEPTH 8
#define MAP_VARIANT 0

#define R1_PIN 42
#define G1_PIN 41
#define B1_PIN 40
#define R2_PIN 38
#define G2_PIN 39
#define B2_PIN 37
#define A_PIN 45
#define B_PIN 36
#define C_PIN 48
#define D_PIN 35
#define E_PIN -1
#define LAT_PIN 47
#define OE_PIN 14
#define CLK_PIN 2

static MatrixPanel_I2S_DMA *dma_display = nullptr;

static void map104x52Coords(VirtualCoords &coords) {
#if MAP_VARIANT == 0
  static const int tile_width = 8;
  static const int tile_height = 13;
  static const int even_vblock_offset = 4;
  static const int odd_vblock_offset = 4;

  const int vert_block_is_odd = (coords.y / tile_height) % 2;
  const int even_vblock_shift = (1 - vert_block_is_odd) * even_vblock_offset;
  const int odd_vblock_shift = vert_block_is_odd * odd_vblock_offset;

  coords.x += ((coords.x + even_vblock_shift) / tile_width) * tile_width + odd_vblock_shift;
  coords.y = (coords.y % tile_height) + tile_height * (coords.y / (tile_height * 2));
#else
  const bool is_top_stripe = (coords.y % (PANEL_RES_Y / 2)) < (PANEL_RES_Y / 4);
  coords.x = (coords.x * 2) + (is_top_stripe ? 1 : 0);
  coords.y = ((coords.y / (PANEL_RES_Y / 2)) * (PANEL_RES_Y / 4)) + (coords.y % (PANEL_RES_Y / 4));
#endif
}

class Panel104x52S13 : public VirtualMatrixPanel {
 public:
  using VirtualMatrixPanel::VirtualMatrixPanel;

 protected:
  VirtualCoords getCoords(int16_t x, int16_t y) override {
    coords = VirtualMatrixPanel::getCoords(x, y);
    if (coords.x == -1 || coords.y == -1) {
      return coords;
    }
    map104x52Coords(coords);
    return coords;
  }
};

static VirtualCoords mappedCoords(int16_t x, int16_t y) {
  VirtualCoords coords;
  coords.x = x;
  coords.y = y;
  map104x52Coords(coords);
  return coords;
}

static Panel104x52S13 *panel = nullptr;

int16_t panelAlignVirtualX(int16_t virtX, int16_t virtY, int16_t refVirtY) {
  const VirtualCoords ref = mappedCoords(virtX, refVirtY);
  if (ref.x < 0) {
    return virtX;
  }

  for (int delta = -64; delta <= 64; delta++) {
    const VirtualCoords candidate = mappedCoords(static_cast<int16_t>(virtX + delta), virtY);
    if (candidate.x == ref.x) {
      return static_cast<int16_t>(virtX + delta);
    }
  }
  return virtX;
}

MatrixPanel_I2S_DMA *panelProfileDma() {
  return dma_display;
}

VirtualMatrixPanel *panelProfileVirtual() {
  return panel;
}

bool panelProfileBegin() {
  HUB75_I2S_CFG::i2s_pins pins = {
    R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };

  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X * 2,
    PANEL_RES_Y / 2,
    PANEL_CHAIN,
    pins
  );
  mxconfig.clkphase = false;
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_16M;
  mxconfig.min_refresh_rate = PANEL_REFRESH_HZ;
  mxconfig.setPixelColorDepthBits(PANEL_COLOR_DEPTH);
  mxconfig.latch_blanking = 1;
  mxconfig.double_buff = true;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if (!dma_display->begin()) {
    return false;
  }

  Serial.printf("Panel refresh rate: %d Hz (target %d Hz)\n",
                dma_display->calculated_refresh_rate, PANEL_REFRESH_HZ);

  dma_display->clearScreen();
  panel = new Panel104x52S13(*dma_display, 1, 1, PANEL_RES_X, PANEL_RES_Y);
  return true;
}
