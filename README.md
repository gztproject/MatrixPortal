# MatrixSign

Firmware for an **Adafruit MatrixPortal ESP32-S3** driving a **P3.076 104×52 px, 1/13 scan** HUB75 LED panel. Configure messages, GIFs, and built-in effects from a phone or laptop over Wi‑Fi.

---

## ⚠️ Vibe-coded project — read this first

**This repository is intentionally “vibe-coded.”** Most of it was designed and written with heavy AI assistance, iterated quickly on real hardware, and optimized for “works on my panel” rather than production rigor.

**What that means in practice:**

- Code structure and APIs may change without notice.
- Error handling, security, and edge cases are incomplete.
- There are no systematic tests; behavior is validated manually on one setup.
- Comments and docs may lag behind the code.
- Default Wi‑Fi credentials are hard-coded for convenience, not security.
- Do **not** treat this as a reference architecture for safety-critical signage (roadworks, emergency egress, etc.).

**Use at your own risk.** If you deploy it, review the code yourself, change defaults (especially AP password), and verify output on **your** panel before relying on it in the field.

Contributions and hardening are welcome, but expect rough edges.

---

## Hardware

| Item | Detail |
|------|--------|
| MCU board | Adafruit MatrixPortal ESP32-S3 |
| Panel | P3.076, **104×52** pixels, **1/13 scan** |
| Buttons | **UP** = GPIO 6, **DOWN** = GPIO 7 (cycle presets) |
| Pixel mapping | Custom `ZnMirrorZStripe`-style map, tile height 13 ([`src/panel_profile.cpp`](src/panel_profile.cpp)) |
| DMA | 16 MHz, 8-bit colour, ~90 Hz refresh |

---

## Quick start

### Build and upload

Requires [PlatformIO](https://platformio.org/).

```bash
pio run -t upload
pio run -t uploadfs   # if you add files under data/ for LittleFS
```

Serial monitor: **115200** baud.

### Connect and configure

1. Power the sign. It always runs a Wi‑Fi access point:
   - **SSID:** `MatrixSign`
   - **Password:** `matrixsign` (change in [`src/wifi_manager.cpp`](src/wifi_manager.cpp) before deployment)
   - **URL:** http://192.168.4.1
2. Open the Web UI in a browser.
3. Optionally connect to home Wi‑Fi from **Home Wi‑Fi (optional)** in the UI; the sign keeps the AP up and uses STA in the background when connected.

On home Wi‑Fi, use the IP shown in the UI status bar (e.g. `http://10.x.x.x`).

---

## Features (current)

### 8 preset slots

Each slot stores one **content type** and its settings in NVS (non-volatile storage):

| Type | Description |
|------|-------------|
| **Message** | Scrolling or static text, 1–4 rows, configurable block height (8–52 px), colour |
| **GIF** | Animated GIF from LittleFS (best **104×52**, max **256 KB** per slot) |
| **Effect** | Built-in full-panel animation (see below) |

- **Save slot** — writes the editor to the selected slot (1–8).
- **Show on panel** — switches the live display to the selected slot.
- **Apply edits to live slot** — when editing a different slot than the one on air, pushes the form to the **currently live** slot only.

**UP / DOWN** buttons on the MatrixPortal cycle the active preset on the device.

### Global brightness

Panel brightness (**1–100%**) is **global**, not per preset. It is stored separately in NVS and applied immediately from the Web UI slider (via `/api/brightness`).

### Built-in effects

| ID | Label | Behaviour (summary) |
|----|-------|---------------------|
| `bright_white` | Bright white | Full panel white |
| `blue_emergency` | Blue emergency | Left/right halves: 2 flashes per side, then switch |
| `yellow_emergency` | Yellow emergency | Same pattern, amber/yellow |
| `arrow_left` | Arrows left | Full-height yellow chevrons stepping horizontally ← |
| `arrow_right` | Arrows right | Same, direction → |
| `stop` | Stop | Red background, white **STOP** (large text) |
| `hazard_triangle` | Hazard triangle | Amber equilateral triangle, red border, **!** |

Effect rendering: [`src/effect_renderer.cpp`](src/effect_renderer.cpp).

### Web UI

Single-page app embedded in firmware ([`src/web_ui.h`](src/web_ui.h)):

- Context-aware banner (AP vs home Wi‑Fi)
- Preset grid with short labels, live/editing/unsaved indicators
- **104×52 preview canvas** (approximate; effects/GIF are simplified)
- Text size cheatsheet (block height × rows → glyph size)
- Scroll speed presets (Slow / Normal / Fast)
- Sticky **Save** / **Show on panel** actions
- Toast notifications for API feedback

### Wi‑Fi

- **AP-first:** `MatrixSign` @ `192.168.4.1` (WPA2)
- **Optional STA:** saved credentials via Web UI; reconnects on boot in background (30 s timeout)
- **Forget home Wi‑Fi** clears saved STA credentials

---

## HTTP API

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/` | Web UI |
| GET | `/api/presets` | All slots + `activeIndex` + `brightness` + Wi‑Fi status |
| POST | `/api/presets` | Save slot (`id` + preset fields) |
| POST | `/api/presets/select` | Activate slot (`id`) |
| GET | `/api/config` | Active preset fields + status |
| POST | `/api/config` | Apply fields to **live** preset |
| GET | `/api/brightness` | Global brightness |
| POST | `/api/brightness` | Set global brightness (`brightness`: 1–100) |
| GET | `/api/effects` | Effect catalog |
| POST | `/api/presets/gif?id=N` | Upload GIF to slot N |
| DELETE | `/api/presets/gif?id=N` | Remove GIF from slot N |
| POST | `/api/wifi/connect` | Save STA credentials and connect |
| POST | `/api/wifi/reset` | Forget STA credentials |

Preset JSON fields (no per-preset brightness): `contentType`, `effectId`, `textHeightPx`, `rowCount`, `text`, `scroll`, `scrollDelayMs`, `color` (`#RRGGBB`).

---

## Software architecture

```
main.cpp
├── panel_profile     HUB75 init + 104×52 pixel mapping
├── preset_store      8 NVS-backed presets + global brightness
├── display_engine    Routes content: effect → GIF → text; scroll tick
├── effect_renderer   Built-in effect catalog + animation
├── gif_player        AnimatedGIF from LittleFS
├── wifi_manager      AP + optional STA
├── web_server        REST API + serves Web UI
└── button_input      UP/DOWN preset cycle
```

**Content priority on a preset:** Effect (if set) → GIF (if file exists) → Text.

**Partitions** ([`partitions.csv`](partitions.csv)): 4 MB app, 2 MB LittleFS (GIF storage under `/gif/`).

---

## Progress log (high level)

Work so far has been iterative “make it work on the bench” development:

- [x] Custom pixel map for 104×52 / 1/13 scan panel
- [x] Stable DMA timing (8-bit, ~90 Hz) for flicker-sensitive text
- [x] 8 NVS preset slots with text, GIF, and effect modes
- [x] Web UI + REST API (AP-first, optional home Wi‑Fi)
- [x] MatrixPortal button preset cycling
- [x] Seven built-in effects (emergency strobes, arrows, STOP, hazard)
- [x] Text layout: pixel block height, multi-row messages, scroll
- [x] Global brightness (decoupled from presets)
- [x] Web UX pass: preview canvas, clearer slot actions, toasts, context banner
- [x] AP WPA2 password
- [ ] OTA updates
- [ ] Automated tests / CI
- [ ] Security hardening (auth on Web UI, unique AP password per device)
- [ ] Native mobile app

---

## Configuration notes

| Setting | Location |
|---------|----------|
| AP SSID / password | [`src/wifi_manager.cpp`](src/wifi_manager.cpp) |
| Default brightness | `GLOBAL_BRIGHTNESS_DEFAULT` in [`src/preset_store.h`](src/preset_store.h) |
| Effect timing / colours | [`src/effect_renderer.cpp`](src/effect_renderer.cpp) |
| GPIO map | [`src/panel_profile.cpp`](src/panel_profile.cpp) |

---

## Dependencies

See [`platformio.ini`](platformio.ini):

- ESP32 HUB75 LED MATRIX PANEL DMA Display
- Adafruit GFX
- ESPAsyncWebServer / AsyncTCP
- ArduinoJson
- AnimatedGIF
- WiFiManager (STA credential storage)

---

## License

No license file is included yet. Treat as **all rights reserved / use at your own risk** until one is added.

---

## Acknowledgements

- [mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) — panel driver and mapping examples
- Adafruit MatrixPortal and GFX libraries
- Built with PlatformIO, Cursor, and a lot of trial and error on a garage bench
