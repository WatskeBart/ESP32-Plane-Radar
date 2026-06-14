# Plane Radar

![plane-radar](https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419)

**Live ADS-B aircraft radar on a 1.28″ round display, powered by an ESP32-C3 Super Mini.**

[**📦 Download firmware**](https://github.com/WatskeBart/ESP32-Plane-Radar/releases) · [**🖨️ 3D printed case (STL + assembly)**](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083)

Firmware for an **ESP32-C3 Super Mini** and a **1.28″ round GC9A01** display (240×240). It shows a circular **ADS-B radar** around your location, using [WiFiManager](https://github.com/tzapu/WiFiManager) for first-time Wi-Fi setup. Once on your network, a built-in web UI (served from LittleFS) takes over for configuration, saved locations, and device controls.

## ✨ Features

- 📡 **Live radar** — real aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid, refreshed every ~3 s
- 🛬 **Runway overlay** — major-airport runways with ICAO labels
- ✈️ **Per-aircraft tags** — callsign, type, and altitude (km/mi, ft/m, all toggleable)
- 🔍 **Watch filter** — show only callsigns matching prefixes you choose (e.g. `KLM,BAW`)
- 🌐 **Built-in web UI** — configure the radar, save up to 3 locations, and manage the device from any browser
- 🎛️ **One-button control** — cycle range presets or reset Wi-Fi from the BOOT button

## 🚀 Flashing the firmware

You need **Chrome or Edge** (Firefox does not support Web Serial).

### Step 1 — Download the firmware

Grab the latest `plane-radar-x.x.x.bin` from the [Releases page](https://github.com/WatskeBart/ESP32-Plane-Radar/releases).

### Step 2 — Put the device into download mode

The ESP32-C3 must be in download mode before it will accept a flash:

1. **Unplug** the device from USB
2. **Hold** the BOOT button (the small button on the board)
3. **Plug** the USB cable back in while still holding BOOT
4. **Release** the BOOT button

The device is now in download mode. It will not show anything on the display.

### Step 3 — Open ESPWebTool

Open [esptool.spacehuhn.com](https://esptool.spacehuhn.com/) in Chrome or Edge.

Click **Connect**, select the device from the port list, and click **Connect** again.

### Step 4 — Erase the flash (first time or after a partition change)

> **Required** if this is a fresh device, or if you are upgrading from an older version of this firmware that used a different partition layout. A plain reflash over the top keeps your saved Wi-Fi and settings, but erasing wipes everything for a clean start.

Click **Erase** and wait for it to complete.

### Step 5 — Flash the firmware

1. Set the **offset** field to `0x0`
2. Click **Choose file** and select the `.bin` you downloaded in Step 1
3. Click **Program**

Wait for the progress bar to finish. When it says "Done", the flash is complete.

### Step 6 — Reboot the device

Unplug and replug the USB cable (no button press needed this time). The radar boots up and shows the setup screen.

### Step 7 — Connect to Wi-Fi

1. On your phone or laptop, connect to the **`PlaneRadar-Setup`** Wi-Fi network that appears
2. Open **`http://192.168.4.1`** in a browser (a captive portal may open automatically)
3. Select your home Wi-Fi network and save

The device reboots, connects, and starts drawing the radar. From then on, browse to **`http://plane-radar.local`** (or the device IP) to set your location and adjust settings. → [Web interface guide](docs/web-ui.md)

---

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
| [🌐 Web interface](docs/web-ui.md) | Setup vs. connected mode, Configure Radar / Locations / Device pages |
| [📡 Radar display](docs/radar-display.md) | Grid, range presets, runways, aircraft symbols, overlays, ADS-B source |
| [⚙️ Configuration](docs/configuration.md) | `config.h` reference and project layout |
| [🛠️ Building & flashing](docs/building.md) | Build from source, merged image, full wipe, CI & releases |

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

## Credits

This project is a fork of [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar) by [MatixYo](https://github.com/MatixYo). All credit for the original concept and implementation goes to them.
