# Changelog

## v0.2.9 — 2026-06-08

### Added

- **Wi‑Fi:** STA-first boot (~15 s), AP fallback with MAC-suffixed SSID (`MatrixSign-XXXX`), STA-loss retry (~120 s) before AP, captive DNS on AP
- **Safe mode:** both buttons at power-on → AP + web UI before panel init; recovery screen, no GIF/playlist
- **Onboard NeoPixel** Wi‑Fi status (yellow/green/red blink/solid patterns; GPIO 4)
- **`GET /api/status`** — heap, uptime, Wi‑Fi mode, recovery/brownout flags
- **Robustness:** HTTP body limits (8 kB / 16 kB restore), GIF upload validation, optional panel brightness cap (`PANEL_BRIGHTNESS_CAP_PERCENT`, default **100%**), brownout boot dim (10%), playlist hardening (1 s dwell min), atomic config restore validation
- **`wifi_config.h`**, **`limits_config.h`**, **`status_led`**

### Changed

- Boot STA failure switches to AP-only **without** clearing saved credentials
- Playlist off via checkbox or `dwellMs: 0`; minimum dwell **1 s**
- AP turns off when STA is connected (no concurrent AP+STA)
- Brownout brightness clamp (10%) clears when user raises brightness above 10% (Web UI or buttons; no reboot)

## v0.2.8 — 2026-06-08

### Changed

- Stable AP-first Wi‑Fi with captive portal redirects for Android/Windows connectivity checks

## v0.2.7 — 2026-06-07

### Fixed

- Scrolling text shear at panel midline: compensate per-row virtual X for the 13-row tile remap so all rows share the same physical scroll step (not just a back-buffer clear)
- Web UI double-click on preset slot: debounce single-click vs double-click and ignore stale `loadPresets` responses that could undo live preset selection

## v0.2.6 — 2026-06-07

### Fixed

- Preset save now persists reliably: serialize all string fields with `.set()`, write to NVS via a fixed buffer, verify `putString`, and only update RAM after a successful write
- Preset NVS access is mutex-protected so concurrent Web UI requests cannot corrupt saves
- Save/restore API returns an error when NVS write fails instead of reporting success
- Restore POST parses JSON with explicit body length
- Scrolling text shear at panel midline: clear the full DMA back buffer before each text redraw instead of a partial virtual fillRect (stale pixels at 13-row tile boundaries showed through with double buffering)
- OTA **Upgrade from URL** rejected valid `{}` body due to inverted JSON parse check

## v0.2.5 — 2026-06-07

### Added

- Display on/off: UP+DOWN hold toggles the panel; Web UI checkbox; off state uses 5% brightness and a red status pixel
- Blue quick color preset (Red, Green, Blue, Amber, White)
- Clock color picker in Web UI (panel already used preset colour)

### Changed

- DMA double buffering for smoother updates; clock redraws only when the formatted time changes

### Fixed

- Preset switch clears both DMA buffers so no leftover artefacts from the previous preset
- Animated effects (chevrons, strobe, pulse, etc.) flip the buffer only when a frame is redrawn (fixes jumping with double buffer)

## v0.2.4 — 2026-06-07

### Fixed

- Preset message text and slot labels persist again: avoid broken ArduinoJson writes to `text`/`label` keys on ESP32, export `message`/`slotLabel`, and parse POST bodies with explicit length

## v0.2.3 — 2026-06-05

### Fixed

- Preset text/labels in Web UI and backup JSON: disable browser caching on preset API responses, detect JSON overflow on load/export, and improve string parsing
- Timezone survives reboot and NTP sync: use `configTzTime()` so SNTP no longer overwrites the saved TZ with UTC

## v0.2.2 — 2026-06-05

### Fixed

- Preset save/load: accumulate POST body so text and slot labels persist in NVS and backup JSON
- Timezone survives reboot (no longer reset to CET on startup)

### Changed

- Caron accents (č/š/ž): shifted 1 px right and 1 px gap above glyph (scaled with text size)

## v0.2.1 — 2026-06-05

### Fixed

- OTA update check: follow GitHub redirects, fetch manifest off async web thread, clearer HTTP errors
- **Check for updates** returns success when already on latest (no HTTP 400)

## v0.2.0 — 2026-06-05

### Added

- Tasmota-style OTA firmware updates (manual `.bin` upload + remote check/upgrade)
- Panel shows **Updating…** with progress % during OTA (web upload or remote download)
- Dual OTA partition layout (`app0` + `app1`)
- Web UI **Firmware upgrade** section (OTA URL, check, upgrade, file upload)
- API: `/api/firmware`, `/api/firmware/url`, `/api/firmware/check`, `/api/firmware/upgrade`, `/api/firmware/upload`
- GitHub Actions release workflow (publishes `firmware.bin` + `version.json` on tag)

### Migration

- **One-time USB flash required** when upgrading from v0.1.x (partition table change)

## v0.1.1 — 2026-06-05

### Fixed

- Web UI login defaults to **admin** / **admin** (change under Security)
- HTTP Basic Auth sends proper 401 (fixes “Handler did not handle the request”)
- Web server stability: serve UI from flash (`send_P`), non-blocking NTP, display engine mutex, larger AsyncTCP buffers
- Home Wi‑Fi connect timeout no longer drops the AP

## v0.1.0 — 2026-06-05

### Security

- HTTP Basic Auth on Web UI and all API routes (username `admin`)
- AP password configurable at build time via `MATRIXSIGN_AP_PASSWORD`; removed from API responses
- `POST /api/auth/password` to change Web UI password
- Added [SECURITY.md](SECURITY.md)

### Documentation

- MIT License
- README: supported hardware, deploy checklist, full API reference
- GitHub Actions CI build

### Features

- Long-press brightness repeat interval 750 ms (10% steps)

## Prior work

- 8 preset slots (text, GIF, effects, clock, countdown)
- Web UI + REST API, playlist, backup/restore
- Timezone selection, browser time sync, text offset
- MatrixPortal button preset cycling and long-press brightness (10% steps)
