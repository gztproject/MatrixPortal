#include "gif_player.h"

#include "panel_profile.h"

#include <AnimatedGIF.h>
#include <FS.h>
#include <LittleFS.h>

namespace {
VirtualMatrixPanel *panel = nullptr;
AnimatedGIF gif;
File gifFile;
bool gifIsOpen = false;
int canvasW = 0;
int canvasH = 0;
int nextFrameDelayMs = 0;
unsigned long nextFrameAtMs = 0;

int mapGifToPanelX(int gifX) {
  if (canvasW <= PANEL_RES_X) {
    return (PANEL_RES_X - canvasW) / 2 + gifX;
  }
  return gifX * PANEL_RES_X / canvasW;
}

int mapGifToPanelY(int gifY) {
  if (canvasH <= PANEL_RES_Y) {
    return (PANEL_RES_Y - canvasH) / 2 + gifY;
  }
  return gifY * PANEL_RES_Y / canvasH;
}

void GIFDraw(GIFDRAW *pDraw) {
  if (!panel) {
    return;
  }

  uint8_t *pixels = pDraw->pPixels;
  uint16_t *palette = pDraw->pPalette;
  const int gifY = pDraw->y;
  const int panelY = mapGifToPanelY(gifY);

  if (panelY < 0 || panelY >= PANEL_RES_Y) {
    return;
  }

  if (pDraw->ucHasTransparency) {
    for (int x = 0; x < pDraw->iWidth; x++) {
      const uint8_t pixel = pixels[x];
      if (pixel == pDraw->ucTransparent) {
        continue;
      }
      const int panelX = mapGifToPanelX(x + pDraw->iX);
      if (panelX >= 0 && panelX < PANEL_RES_X) {
        panel->drawPixel(panelX, panelY, palette[pixel]);
      }
    }
    return;
  }

  for (int x = 0; x < pDraw->iWidth; x++) {
    const int panelX = mapGifToPanelX(x + pDraw->iX);
    if (panelX >= 0 && panelX < PANEL_RES_X) {
      panel->drawPixel(panelX, panelY, palette[pixels[x]]);
    }
  }
}

void *GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = LittleFS.open(fname, "r");
  if (!gifFile) {
    return nullptr;
  }
  *pSize = static_cast<int32_t>(gifFile.size());
  return &gifFile;
}

void GIFCloseFile(void *pHandle) {
  File *file = static_cast<File *>(pHandle);
  if (file != nullptr) {
    file->close();
  }
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *file = static_cast<File *>(pFile->fHandle);
  int32_t bytesToRead = iLen;
  if ((pFile->iSize - pFile->iPos) < iLen) {
    bytesToRead = pFile->iSize - pFile->iPos - 1;
  }
  if (bytesToRead <= 0) {
    return 0;
  }
  bytesToRead = static_cast<int32_t>(file->read(pBuf, bytesToRead));
  pFile->iPos = file->position();
  return bytesToRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *file = static_cast<File *>(pFile->fHandle);
  file->seek(iPosition);
  pFile->iPos = static_cast<int32_t>(file->position());
  return pFile->iPos;
}
}  // namespace

bool gifPlayerBegin(VirtualMatrixPanel *virtualPanel) {
  panel = virtualPanel;
  gif.begin(LITTLE_ENDIAN_PIXELS);
  return true;
}

void gifPlayerEnd() {
  gifPlayerClose();
  panel = nullptr;
}

bool gifPlayerFileExists(const char *path) {
  return path && path[0] != '\0' && LittleFS.exists(path);
}

bool gifPlayerOpen(const char *path) {
  gifPlayerClose();
  if (!path || path[0] == '\0' || !LittleFS.exists(path)) {
    return false;
  }

  if (!gif.open(path, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    return false;
  }

  canvasW = gif.getCanvasWidth();
  canvasH = gif.getCanvasHeight();
  if (canvasW <= 0 || canvasH <= 0 || canvasW > 208 || canvasH > 104) {
    gif.close();
    return false;
  }

  gifIsOpen = true;
  nextFrameDelayMs = 0;
  nextFrameAtMs = 0;
  panel->fillScreen(0);
  return true;
}

void gifPlayerClose() {
  if (gifIsOpen) {
    gif.close();
    gifIsOpen = false;
  }
  if (gifFile) {
    gifFile.close();
  }
  canvasW = 0;
  canvasH = 0;
}

bool gifPlayerIsOpen() {
  return gifIsOpen;
}

bool gifPlayerTick() {
  if (!gifIsOpen) {
    return false;
  }

  const unsigned long now = millis();
  if (now < nextFrameAtMs) {
    return false;
  }

  nextFrameDelayMs = gif.playFrame(true, nullptr);
  if (nextFrameDelayMs <= 0) {
    gif.reset();
    nextFrameDelayMs = 10;
  }
  nextFrameAtMs = now + static_cast<unsigned long>(nextFrameDelayMs);
  return true;
}
