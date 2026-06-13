# Plane Radar

![plane-radar](https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419)

**Live ADS-B aircraft radar on a 1.28″ round display, powered by an ESP32-C3 Super Mini.**

[**📦 Download firmware**](https://github.com/WatskeBart/ESP32-Plane-Radar/releases) · [**🖨️ 3D printed case (STL + assembly)**](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083)

Firmware for an **ESP32-C3 Super Mini** and a **1.28″ round GC9A01** display (240×240). It shows a circular **ADS-B radar** around your location, using [WiFiManager](https://github.com/tzapu/WiFiManager) for first-time Wi-Fi setup. Once on your network, a built-in web UI (served from LittleFS) takes over for configuration, saved locations, OTA firmware updates, and device controls.

## ✨ Features

- 📡 **Live radar** — real aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid, refreshed every ~3 s
- 🛬 **Runway overlay** — major-airport runways with ICAO labels
- ✈️ **Per-aircraft tags** — callsign, type, and altitude (km/mi, ft/m, all toggleable)
- 🔍 **Watch filter** — show only callsigns matching prefixes you choose (e.g. `KLM,BAW`)
- 🌐 **Built-in web UI** — configure the radar, save up to 3 locations, and manage the device from any browser
- ⬆️ **Over-the-air updates** — dual-partition OTA from the Device page, no cable needed
- 🎛️ **One-button control** — cycle range presets or reset Wi-Fi from the BOOT button

## 🚀 Quick Start

1. **Flash** the latest [release `.bin`](https://github.com/WatskeBart/ESP32-Plane-Radar/releases) at `0x0` (ESP32-C3, 4 MB) — see [Building & flashing](docs/building.md) for the merged image and a web flasher. *(Or build it yourself: `pio run -e supermini -t upload` then `pio run -e supermini -t uploadfs`.)*
2. **Connect** your phone/laptop to the **`PlaneRadar-Setup`** Wi-Fi AP that appears on first boot.
3. **Open** `http://plane-radar.local` (or `http://192.168.4.1`), pick your home Wi-Fi, and save.

The device reboots, connects, and starts drawing the radar. From then on, browse to **`http://plane-radar.local`** (or the device IP) to set your latitude/longitude and tweak everything else. → [Web interface guide](docs/web-ui.md)

## 🎛️ Controls (BOOT button, GPIO 9, active LOW)

| Action | Effect |
| --- | --- |
| **Short tap** | Cycle range preset (4 user-configurable steps); saved to flash |
| **Hold 3 s** | Clear Wi‑Fi, location, and units; reboot into setup portal |

During setup you can also hold BOOT at power-on to force a credential reset (same as the long press).

## 📖 Documentation

| Topic | What's inside |
| --- | --- |
| [🔌 Hardware & wiring](docs/hardware.md) | GC9A01 ↔ ESP32-C3 pin map, 3D case |
| [🌐 Web interface](docs/web-ui.md) | Setup vs. connected mode, Configure Radar / Locations / Device pages, OTA |
| [📡 Radar display](docs/radar-display.md) | Grid, range presets, runways, aircraft symbols, overlays, ADS-B source |
| [⚙️ Configuration](docs/configuration.md) | `config.h` reference and project layout |
| [🛠️ Building & flashing](docs/building.md) | Build, debug build, merged image, full wipe, CI & releases |

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
