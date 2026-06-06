# Security

MatrixSign is hobby firmware for a garage LED sign — not certified signage. Read this before deploying on a network others can reach.

## Threat model

| Attacker | Risk (v0.2+) |
|----------|----------------|
| Stranger not on your Wi‑Fi | Low |
| Client on AP or home LAN without Web UI password | Blocked (HTTP Basic Auth) |
| Client with Web UI password | Full control (by design) |
| Wrong panel hardware | Garbled display — verify on your hardware |

## Credentials

### Wi‑Fi access point (AP)

- SSID: `MatrixSign` (fixed in firmware)
- Password: set at **build time** via `MATRIXSIGN_AP_PASSWORD` in `platformio.ini`
- Default dev password `matrixsign` is **not safe for field use**
- The AP password is **never** returned by the HTTP API

### Web UI (HTTP Basic Auth)

- Username: `admin` (fixed)
- Password: default **`admin`** / **`admin`** on first boot (stored in NVS); change in Web UI **Security** (new password min 8 characters)
- Change via **Security** in the Web UI or `POST /api/auth/password` (min 8 characters, authenticated)

## Network exposure

- The device runs **AP + optional STA** at the same time. When home Wi‑Fi is connected, the sign is reachable on **both** the AP IP (`192.168.4.1`) and the STA IP.
- All routes (`/` and `/api/*`) require Basic Auth.
- Home Wi‑Fi credentials are stored by WiFiManager in NVS (ESP32 standard behaviour).

## Not suitable for

- Safety-critical or regulated signage (roadworks, emergency egress, etc.)
- Untrusted public networks without changing defaults and isolating the device

## Reporting issues

Open a GitHub issue with steps to reproduce. Do not post live passwords or Wi‑Fi credentials.
