# MatrixSign

Firmware for an **Adafruit MatrixPortal ESP32-S3** driving a **P3.076 104×52 px, 1/13 scan** HUB75 LED panel. Configure messages, GIFs, clock/countdown, and built-in effects from a phone or laptop over Wi‑Fi.

> **Supported hardware:** Adafruit MatrixPortal ESP32-S3 + **P3.076 104×52, 1/13 scan** panel only. Other panels need a new pixel map in [`src/panel_profile.cpp`](src/panel_profile.cpp).

See [SECURITY.md](SECURITY.md) before deploying on a network others can use.

---

## ⚠️ Vibe-coded project — read this first

**This repository is intentionally “vibe-coded.”** Most of it was designed and written with heavy AI assistance, iterated quickly on real hardware, and optimized for “works on my panel” rather than production rigor.

**What that means in practice:**

- Code structure and APIs may change without notice.
- Error handling and edge cases are incomplete.
- There are no systematic unit tests; behaviour is validated manually on one setup.
- Do **not** treat this as a reference architecture for safety-critical signage (roadworks, emergency egress, etc.).

**Use at your own risk.** Review the code, set credentials before field use, and verify output on **your** panel.

Contributions welcome — see [LICENSE](LICENSE) (MIT).

---

## Hardware

| Item | Detail |
|------|--------|
| MCU board | Adafruit MatrixPortal ESP32-S3 |
| Panel | P3.076, **104×52** pixels, **1/13 scan** |
| Buttons | **UP** = GPIO 6, **DOWN** = GPIO 7 |
| Status LED | Onboard **NeoPixel** on **GPIO 4** (1× RGB, driven by [`src/status_led.cpp`](src/status_led.cpp)) |
| Pixel mapping | Custom `ZnMirrorZStripe`-style map, tile height 13 ([`src/panel_profile.cpp`](src/panel_profile.cpp)) |
| DMA | 16 MHz, 8-bit colour, ~90 Hz refresh |

**NeoPixel — Wi‑Fi status** (500 ms blink, solid when steady):

| LED | Meaning |
|-----|---------|
| Yellow blink | Saved home Wi‑Fi, searching at boot |
| Green blink | STA connecting / reconnecting after link loss |
| Green solid | STA connected |
| Red blink | AP active, waiting for a client |
| Red solid | At least one client connected to AP |

Off during early boot until Wi‑Fi init starts. Safe mode uses red blink/solid like normal AP mode.

**Button behaviour:**

- **Short press UP / DOWN** — cycle to previous / next preset slot.
- **Long press (~600 ms) UP / DOWN** — increase / decrease global brightness by 10% (repeats while held; stops at 1% / 100%).

---

## First-time deploy checklist

1. **Set AP password** in `platformio.ini` before flashing (see [Configuration](#configuration-notes)).
2. Build and upload: `pio run -t upload`
3. Join Wi‑Fi **`MatrixSign-XXXX`** (MAC suffix) with your build-time AP password.
4. Open **http://192.168.4.1** — log in with **admin** / **admin** (change under Security before field use).
5. Configure slots; use **Sync time from this device** if no home Wi‑Fi (clock / target countdown).
6. Optionally connect home Wi‑Fi; when STA is connected the AP is **off** (AP-only fallback if STA fails).
7. Verify messages and effects on **your** panel before leaving unattended.

**Safe mode:** hold **both** UP and DOWN buttons during power-on — AP and web UI start before panel init; STA is skipped; recovery message at low brightness; no GIF/playlist until a normal reboot.

If login fails after an older firmware build (random password stored in NVS), run `pio run -t erase` then upload again.

**Upgrading to v0.2.0+:** dual OTA partitions require a **one-time USB flash** — OTA cannot migrate from the old single-app layout.

---

## Quick start

### Build and upload

Requires [PlatformIO](https://platformio.org/).

```bash
pio run -t upload
```

Serial monitor: **115200** baud.

**Before field use**, add a unique AP password to [`platformio.ini`](platformio.ini):

```ini
build_flags = -DMATRIXSIGN_AP_PASSWORD=\"your-long-ap-password\"
```

### Connect and configure

1. Power the sign. If saved home Wi‑Fi is unavailable it runs an access point:
   - **SSID:** `MatrixSign-XXXX` (last two bytes of MAC)
   - **Password:** your `MATRIXSIGN_AP_PASSWORD` build flag (dev default `matrixsign` — change before deploy)
   - **URL:** http://192.168.4.1
2. Log in to the Web UI: **`admin`** / **`admin`** (change under Security before field use).
3. Optionally connect to home Wi‑Fi from **Home Wi‑Fi (optional)**; when connected the AP turns off until STA is lost.

On home Wi‑Fi, use the IP shown in the UI status bar (e.g. `http://10.x.x.x`).

---

## Features (current)

### 8 preset slots

Each slot stores one **content type** and its settings in NVS (non-volatile storage):

| Type | Description |
|------|-------------|
| **Message** | Scrolling or static text, 1–4 rows, glyph height 8–48 px, colour; UTF‑8 **č, š, ž**; optional X/Y text offset |
| **Clock** | HH:MM (optional seconds, optional EU date); needs time via NTP or browser sync |
| **Countdown** | Target time (needs time sync) or **duration** (starts when slot is shown; no Wi‑Fi) |
| **GIF** | Animated GIF from LittleFS (best **104×52**, max **256 KB** per slot) |
| **Effect** | Built-in full-panel animation (see below) |

Each slot also has an optional **label** (shown in the preset grid). **Duplicate slot** copies a slot’s settings and GIF file to another slot.

**Sticky bar actions:**

- **Try on panel** — show editor content on the panel without saving (`POST /api/preview`).
- **Show saved** — activate the saved slot on the panel.
- **Save slot** — write the editor to the selected slot (1–8).

### Time and timezone

- **Timezone** (CET, UTC, WET, EET, GMT) applies to clock and target countdown.
- **NTP** when home Wi‑Fi is connected (`pool.ntp.org`).
- **Browser sync** (`POST /api/time/sync`) when NTP is unavailable (AP-only field mode).

### Playlist (auto-rotate)

Uncheck **Auto-rotate presets** to turn rotation off (slot selection is unchanged). When on, checked slots rotate after a dwell time (**1 s** minimum; `dwellMs: 0` via API turns playlist off). Manual UP/DOWN still switches slots; long press still adjusts brightness.

### Global brightness

Panel brightness (**1–100%**) is **global**, not per preset. Adjust from the Web UI slider or long-press UP/DOWN on the device. Stored in NVS via `/api/brightness`. After a brownout reset, brightness starts capped at **10%** until you raise it above 10% (no reboot needed).

### Built-in effects

| ID | Label | Behaviour (summary) |
|----|-------|---------------------|
| `bright_white` | Bright white | Full panel solid fill |
| `flashing_halves` | Flashing halves | Left/right halves flash alternately |
| `full_strobe` | Full strobe | Full-panel flash |
| `pulse` | Pulse | Brightness pulse |
| `border_chase` | Border chase | Animated border |
| `progress_bar` | Progress bar | Horizontal fill; level set by `effectParam` (0–100) |
| `game_of_life` | Game of Life | 2×2 px cells, Conway’s rules |
| `arrow_left` | Arrows left | Full-height chevrons stepping ← |
| `arrow_right` | Arrows right | Same, direction → |
| `stop` | Stop | Red background, white **STOP** (fixed colours) |
| `hazard_triangle` | Hazard triangle | Amber triangle, red border, **!** (fixed colours) |

Monochrome effects (all except STOP and hazard) use the preset **colour** field.

Effect rendering: [`src/effect_renderer.cpp`](src/effect_renderer.cpp).

### Firmware upgrade (OTA)

Tasmota-style updates in the Web UI **Firmware upgrade** section:

- **Upgrade from file** — upload a `.bin` over the AP (works without home Wi‑Fi)
- **Check for updates** — fetches `{otaUrl}version.json` (needs home Wi‑Fi + internet)
- **Upgrade from URL** — downloads `firmware.bin` from the manifest and reboots

Default OTA URL points at GitHub Releases (`FIRMWARE_OTA_URL_DEFAULT` in [`platformio.ini`](platformio.ini)); override in the Web UI or at build time.

Publish a release: tag `v0.2.0` → GitHub Actions uploads `firmware.bin` + `version.json`.

The panel shows **Updating…** with progress during web upload or remote download.

### Backup and restore

Download a JSON backup of all slots, labels, playlist, brightness, and timezone. Restore overwrites device configuration from a backup file.

### Web UI

Single-page app embedded in firmware ([`src/web_ui.h`](src/web_ui.h)):

- HTTP Basic Auth on all pages and API calls
- Context-aware banner (AP vs home Wi‑Fi)
- Preset grid with labels, live/editing/unsaved indicators
- Optional slot label, duplicate, playlist controls
- **104×52 preview canvas** (approximate; effects/GIF are simplified)
- Timezone, browser time sync, security (change Web UI password)
- Firmware upgrade (manual upload + remote check)
- Sticky **Try on panel** / **Show saved** / **Save slot**
- Toast notifications for API feedback

### Wi‑Fi

- **STA-first at boot:** tries saved home Wi‑Fi (~15 s); on success AP is off
- **Boot STA fail:** STA is disabled and AP starts; saved credentials are **kept** for the next reboot
- **AP fallback:** `MatrixSign-XXXX` @ `192.168.4.1` (WPA2, build-time password, max 2 clients)
- **STA loss:** retries ~120 s before AP fallback (credentials kept)
- **Safe mode:** both buttons held at power-on → **AP starts immediately** (before panel init), no STA attempt, low brightness, no GIF/playlist
- **Forget home Wi‑Fi** clears saved STA credentials

Wi‑Fi status is shown on the onboard NeoPixel — see [Hardware](#hardware).

### Boot procedure (normal)

1. Panel init → LittleFS → load NVS config  
2. Try saved home Wi‑Fi (~15 s); on success STA-only (AP off)  
3. On STA fail → AP-only fallback (credentials **kept**)  
4. Web server starts → active preset on panel  
5. If STA drops later → retry ~120 s, then AP fallback  

Safe mode (both buttons at power-on) skips steps 1–2 for Wi‑Fi: AP + HTTP first, then panel + recovery screen.

### Robustness (field use)

- **Panel brightness cap:** optional hardware limit via `PANEL_BRIGHTNESS_CAP_PERCENT` (default **100%** — no practical limit)
- **Brownout boot:** after `ESP_RST_BROWNOUT`, effective brightness is capped at **10%** until you raise brightness above 10% (Web UI or long-press UP)
- **HTTP body limits:** 8 kB (most JSON endpoints), 16 kB (`/api/restore`); oversize → HTTP 413
- **GIF upload:** GIF87a/GIF89a header + decode validation; broken uploads are rejected
- **Config restore:** validated before NVS write (invalid backup returns 400, no partial apply)

---

## HTTP API

All endpoints require **HTTP Basic Auth** (username `admin`, password from device).

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/` | Web UI |
| GET | `/api/status` | Diagnostics: heap, uptime, Wi‑Fi mode, recovery flags |
| GET | `/api/presets` | All slots + status (see global fields below) |
| POST | `/api/presets` | Save slot (`id` + preset fields) |
| POST | `/api/presets/select` | Activate slot (`id`) |
| POST | `/api/presets/duplicate` | Copy slot (`from`, `to`) |
| POST | `/api/preview` | Show editor content on panel without saving |
| POST | `/api/playlist` | Set playlist (`enabled`, `slotMask`, `dwellMs`; `0` = off) |
| GET | `/api/backup` | Export full configuration JSON |
| POST | `/api/restore` | Restore from backup JSON |
| GET | `/api/config` | Active preset fields + status |
| POST | `/api/config` | Apply fields to **live** preset (legacy) |
| GET | `/api/brightness` | Global brightness |
| POST | `/api/brightness` | Set global brightness (`brightness`: 1–100) |
| GET | `/api/effects` | Effect catalog |
| POST | `/api/presets/gif?id=N` | Upload GIF to slot N |
| DELETE | `/api/presets/gif?id=N` | Remove GIF from slot N |
| POST | `/api/timezone` | Set timezone (`timezoneId`: CET, UTC, WET, EET, GMT) |
| POST | `/api/time/sync` | Set time from browser (`unix`: seconds UTC) |
| POST | `/api/auth/password` | Change Web UI password (`password`, min 8 chars) |
| GET | `/api/firmware` | Firmware version, OTA URL, update state/progress |
| POST | `/api/firmware/url` | Save OTA base URL (`otaUrl`) |
| POST | `/api/firmware/check` | Fetch remote `version.json` and compare |
| POST | `/api/firmware/upgrade` | Download and install update (optional `{ "url": "..." }`) |
| POST | `/api/firmware/upload` | Upload `.bin` firmware (multipart) |
| POST | `/api/wifi/connect` | Save STA credentials and connect |
| POST | `/api/wifi/reset` | Forget STA credentials |

**Preset JSON fields:** `contentType`, `effectId`, `textHeightPx`, `rowCount`, `text`, `label`, `scroll`, `scrollDelayMs`, `color` (`#RRGGBB`), `effectParam`, `countdownEndUnix`, `countdownDurationSec`, `contentOffsetX`, `contentOffsetY`.

**Clock `effectParam` flags:** bit 0 = show seconds, bit 2 = show date (`dd.mm.yyyy`). For effects, `effectParam` is 0–100 (e.g. progress bar level).

**Global fields in GET `/api/presets`:** `brightness`, `playlistEnabled`, `playlistMask`, `playlistDwellMs`, `timezoneId`, `timeValid`, `timeSource`, `firmwareVersion`, `otaUrl`, `otaState`, `otaProgress`, `otaUpdateAvailable`, `apSsid`, `apIp`, `staConnected`, `staIp`, `staRssi`.

---

## Software architecture

```
main.cpp
├── panel_profile     HUB75 init + 104×52 pixel mapping
├── preset_store      8 NVS-backed presets + global brightness + playlist + timezone
├── display_engine    Content routing, scroll, clock/countdown, playlist tick, recovery mode
├── effect_renderer   Built-in effect catalog + animation
├── text_renderer     Bitmap font + UTF-8 č/š/ž
├── time_sync         SNTP + browser sync + POSIX timezones
├── gif_player        AnimatedGIF from LittleFS
├── wifi_manager      STA-first / AP fallback, captive DNS on AP
├── status_led        Onboard NeoPixel Wi‑Fi status
├── web_auth          HTTP Basic Auth password (NVS)
├── ota_update        OTA upload + remote upgrade
├── web_server        REST API + serves Web UI
└── button_input      Short press: presets; long press: brightness; boot: safe mode
```

**Partitions** ([`partitions.csv`](partitions.csv)): dual OTA app slots (~1.9 MB each), ~4.2 MB LittleFS for GIFs.

---

## Configuration notes

| Setting | Location |
|---------|----------|
| Firmware version | `FIRMWARE_VERSION` in [`platformio.ini`](platformio.ini) |
| Default OTA URL | `FIRMWARE_OTA_URL_DEFAULT` in [`platformio.ini`](platformio.ini) |
| AP password (build time) | `build_flags = -DMATRIXSIGN_AP_PASSWORD=\"...\"` in [`platformio.ini`](platformio.ini) |
| AP SSID prefix | [`src/wifi_manager.cpp`](src/wifi_manager.cpp) (`MatrixSign-XXXX`) |
| Wi‑Fi / recovery timeouts | [`src/wifi_config.h`](src/wifi_config.h) |
| Panel brightness cap (default 100%) | `PANEL_BRIGHTNESS_CAP_PERCENT` in [`src/limits_config.h`](src/limits_config.h) |
| HTTP body limits / brownout dim | [`src/limits_config.h`](src/limits_config.h) |
| Web UI password | Default **`admin`**; change in Web UI **Security** (min 8 characters) |
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

[MIT License](LICENSE) — Copyright (c) 2026 MatrixSign contributors.

---

## Acknowledgements

- [mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) — panel driver and mapping examples
- Adafruit MatrixPortal and GFX libraries
- Built with PlatformIO, Cursor, and a lot of trial and error on a garage bench
