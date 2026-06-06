# Changelog

## v0.1.0 — 2026-06-05

### Security

- HTTP Basic Auth on Web UI and all API routes (username `admin`, random NVS password on first boot)
- AP password configurable at build time via `MATRIXSIGN_AP_PASSWORD`; removed from API responses
- `POST /api/auth/password` to change Web UI password
- Added [SECURITY.md](SECURITY.md)

### Documentation

- MIT License
- README: supported hardware, deploy checklist, full API reference
- GitHub Actions CI build

## v0.1.0 — prior work

- 8 preset slots (text, GIF, effects, clock, countdown)
- Web UI + REST API, playlist, backup/restore
- Timezone selection, browser time sync, text offset
- MatrixPortal button preset cycling and long-press brightness (10% steps)
