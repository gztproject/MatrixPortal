# Changelog

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
