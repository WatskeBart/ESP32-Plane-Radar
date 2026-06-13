# Plane Radar

![plane-radar](https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419)

**3D printed case (STL + assembly):** [MakerWorld](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083) · **Firmware:** [Releases](https://github.com/WatskeBart/ESP32-Plane-Radar/releases)

Firmware for an **ESP32-C3 Super Mini** and a **1.28″ round GC9A01** display (240×240). Shows a circular **ADS-B radar** around your configured location, with **WiFiManager** for first-time setup. Once on your network, a built-in web UI (served from LittleFS) takes over for configuration, saved locations, OTA firmware updates, and device controls.

## What it does

1. **Wi‑Fi setup** (if needed) — captive portal on AP **`PlaneRadar-Setup`** (WiFiManager)
2. **Radar** — live aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid
3. **Web UI** (once connected) — own web server serving the radar/locations/device pages

After Wi‑Fi is saved, the device reconnects automatically; the radar runs in the main loop with periodic ADS-B updates (~3 s). WiFiManager's pages are shown **only** in setup (AP) mode — once connected, only the custom web UI is served.

## Controls (BOOT, GPIO 9, active LOW)

| Action | Effect |
| --- | --- |
| **Short tap** | Cycle range preset (4 user-configurable steps); saved to flash |
| **Hold 3 s** | Clear Wi‑Fi, location, and units; reboot into setup portal |

During setup you can also hold BOOT at power-on to force a credential reset (same as the long press).

## Web interface

The device runs **two different web UIs depending on its mode**:

| Mode | Server | URL | Purpose |
| --- | --- | --- | --- |
| **Setup (AP)** | WiFiManager | `http://plane-radar.local` / `http://192.168.4.1` | Join your Wi‑Fi (only when there are no/invalid credentials, or after a reset) |
| **Connected (STA)** | Built-in radar UI (LittleFS) | `http://plane-radar.local` / `http://<device-ip>` | Menu → Configure Radar, Locations, Device/OTA |

Once connected to your network, WiFiManager's pages are no longer served — only the custom radar UI is reachable.

### Setup (AP) mode

1. Connect to the **`PlaneRadar-Setup`** Wi‑Fi AP
2. Open **`http://plane-radar.local`** or **`http://192.168.4.1`** (both shown on the yellow setup screen; the captive portal may open automatically)
3. Select your home Wi‑Fi and save — the device reboots and connects

Or hardcode fallback AP credentials in [config.h](include/config.h#L24).

### Connected (STA) mode

Browse to **`http://plane-radar.local`** — or the device IP from your router. The hostname is `kPortalHostname` in [config.h](include/config.h#L21), the single name used for the setup AP, DHCP, and mDNS. Some clients resolve `.local` slowly — use the IP if needed. If you run more than one device on the same LAN, give each a distinct `kPortalHostname` to avoid a name clash.

The root path (`/`) is a **menu** with three buttons; each page has a **← Back to menu** link.

Pages: **Configure Radar** (`/radar`), **Locations** (`/locs`), **Device** (`/device`).

#### Configure Radar (`/radar`)

Saving returns to this page (not the main menu), so settings can be tweaked without navigating back.

| Field | Purpose |
| --- | --- |
| **Latitude / Longitude** | Radar center and ADS-B query position (defaults in `config.h` until set) |
| **Display distances in miles** | Ring scale label in **mi** instead of **km** (e.g. `6mi` vs `10km`) |
| **Show airport runways** | Major-airport runway overlay on the radar |
| **Show aircraft count on display** | Shows the number of visible aircraft at the south edge of the radar |
| **Show flight number / callsign** | Toggle the top tag line (white text) per aircraft symbol |
| **Show aircraft type** | Toggle the middle tag line (yellow text, e.g. `B738`) |
| **Show altitude** | Toggle the bottom tag line (blue text) |
| **Show altitude in metres** | Converts altitude tag from feet to metres (e.g. `35000 ft` → `10668m`) |
| **Watch callsign prefixes** | Comma-separated list of prefixes (e.g. `KLM,BAW`); only matching aircraft are shown — all others are hidden |
| **Range presets** | Four configurable ring-3 distances in km (must be strictly increasing, 0.5–500 km) |

#### Locations (`/locs`)

Save up to **3 named locations** in flash. Use the **Use** button to switch the radar center to a saved location; **Delete** to remove it. Adding a new manual lat/lon on the main radar page clears the active saved location.

#### Device (`/device`)

The **Info** panel shows the running **firmware version** (git tag/commit, injected at build by [scripts/git_version.py](scripts/git_version.py)), hostname, mDNS name, IP, and connected network — handy for confirming an OTA actually took.

| Action | Effect |
| --- | --- |
| **Firmware update (OTA)** | Upload a `firmware.bin` to flash over the air; the device reboots into the new build |
| **Restart device** | Soft reboot |
| **Reset WiFi credentials** | Clears **only** the saved Wi‑Fi (keeps locations, units, presets) and reboots into setup AP mode |

OTA uses the dual app partitions (`app0`/`app1`); upload the plain **`firmware.bin`** (`.pio/build/supermini/firmware.bin`), **not** the merged image. The BOOT **hold 3 s** does a fuller reset (Wi‑Fi + location + units), whereas the Device page's reset is Wi‑Fi-only.

After a reset, the device reboots and shows the setup screen immediately (no "Connecting" loop on stale credentials).

## Radar display

### Grid

- Dark blue background, subdued green rings and crosshairs
- White **N / S / E / W** at the bezel; range label on the **east** spoke (ring 3 = ¾ of outer radius)
- White center dot

Layout and colors: [`include/ui/radar_theme.h`](include/ui/radar_theme.h).

### Range presets

Four range presets are configurable via the portal (`/radar` → Range presets). The default values are:

| Ring 3 label | Outer radius (aircraft scale) |
| --- | --- |
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Presets and miles/km choice persist across reboot (`planeradar` NVS namespace). Tap BOOT to cycle presets on-device.

### Runways

- Major airports from OurAirports (`large_airport`); all open runway strips in range (helipads excluded)
- Teal runway lines with one ICAO label per airport (e.g. `KJFK`); toggle in the Wi‑Fi setup portal
- Update the embedded list: `python3 scripts/build_large_airports.py`

### Aircraft

- **Inside the outer ring** — red heading triangle, magenta speed vector (clipped at the ring), callsign / type / altitude tags
- **Outside the ring** (still within ADS-B fetch) — small **red dot on the screen rim** at the correct bearing (direction cue; not distance-accurate past the ring)
- **Tags** — placed toward the **center**: west (left) → tag on the **right** of the symbol; east (right) → tag on the **left**; each of the three lines can be hidden individually via the portal
- **Watch filter** — when prefixes are configured, only aircraft whose callsign starts with a matching prefix are shown; all others are hidden

As range decreases (or aircraft approach), targets move inward; beyond-ring dots become full symbols when they cross the outer ring.

### Aircraft count overlay

When enabled via the portal, the number of visible aircraft is shown in bright green at the bottom of the radar disc. When a watch filter is active, the count reflects only the matching aircraft. Toggle with the **Show aircraft count on display** checkbox.

### Stale data indicator

A small dot in the top-right corner (between the outer ring and the screen edge) shows API health:

| Color | Meaning |
| --- | --- |
| None (hidden) | Data is fresh (< 10 s old) |
| Orange | Last successful fetch was 10–30 s ago |
| Red | Last successful fetch was > 30 s ago or never |

### ADS-B

- Source: `https://opendata.adsb.fi/api/v3/`
- Fetch radius: `ui::radar::fetchRadiusKm()` — scales with the active preset to roughly the screen edge (so rim dots have data)
- Poll interval: [`kAdsbFetchIntervalMs`](include/config.h#L55) (3 s)
- Ground aircraft hidden by default [`kAdsbShowGroundAircraft`](include/config.h#L59)

## Configuration

Edit **[`include/config.h`](include/config.h)** for hardware and behavior:

| Area | Keys / notes |
| --- | --- |
| Portal / hostname | `kPortalApName`, `kPortalIp`, `kPortalHostname` (single hostname for the setup AP **and** the LAN/mDNS in STA mode) / `kPortalHostUrl`; needs `-DWM_MDNS` in `platformio.ini` |
| Wi‑Fi timing | connect attempts, reconnect grace, portal timeout (`0` = no timeout) |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| Display SPI | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |

Default range presets: `include/ui/radar_range.h` (`kDefaultRangePresets`). These are compile-time fallbacks; runtime values are configurable via the portal and stored in NVS.

## Project layout

```text
include/
  config.h
  hardware/
    lgfx_config.hpp
    display.h
    display_font.h
  data/
    large_airports.h
  ui/
    radar_theme.h
    radar_range.h
    radar_display.h
    runway_overlay.h
    status_screens.h
  services/
    wifi_setup.h
    radar_location.h
    adsb_client.h
    web_portal.h
data/
  ui_font.vlw              — embedded smooth UI font (Noto Sans Bold)
web/                       — LittleFS web assets (built into the FS image)
  index.html               — root menu (Configure Radar / Locations / Device)
  radar.html  radar.css  radar.js
  locs.html   locs.js
  device.html device.js
scripts/
  build_large_airports.py
src/
  main.cpp
  data/
    large_airports_data.cpp
  hardware/
  ui/
  services/
    web_portal.cpp         — own web server: serves web/ + JSON API + OTA (STA mode)
```

## Wiring (GC9A01 ↔ ESP32-C3 Super Mini)

| Display | ESP32-C3 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| RST | GPIO **0** |
| CS | GPIO **1** |
| DC | GPIO **10** |
| SDA (MOSI) | GPIO **3** |
| SCL (SCLK) | GPIO **4** |
| BOOT (user) | GPIO **9** |

## Build

```bash
pio run -e supermini -t upload          # firmware
pio run -e supermini -t uploadfs        # web UI assets (LittleFS, from web/)
pio device monitor
```

- PlatformIO env: **`supermini`**
- Serial: **115200** baud
- USB CDC on boot enabled in `platformio.ini` for the Super Mini
- The web UI (`web/`) is flashed to the **LittleFS** partition via `uploadfs`; edit files in `web/` and re-run `uploadfs` to update them without rebuilding firmware
- Flash layout is **dual-OTA** (`partitions/plane_radar.csv`): two ~1.81 MB app slots (`app0`/`app1`) so the Device page can update firmware over the air, plus a 256 KB LittleFS partition

### Debug build

A second environment enables verbose render-pipeline logging to the serial monitor:

```bash
pio run -e supermini-debug -t upload
pio device monitor
```

This adds `-DRADAR_DISPLAY_DEBUG` and prints per-frame checkpoints (`renderFrame start`, `grid done`, `pushSprite done`, etc.) without affecting the normal `supermini` build.

### Web-flashable release image

Single `.bin` for [esptool-js](https://espressif.github.io/esptool-js/) and similar tools (ESP32-C3, 4 MB, flash at **0x0**):

```bash
chmod +x scripts/merge-firmware.sh   # once
./scripts/merge-firmware.sh
```

Writes `release/plane-radar-merged.bin`. Skip rebuild if firmware is already built:

```bash
./scripts/merge-firmware.sh --no-build
```

Or via PlatformIO only (output: `.pio/build/supermini/firmware-merged.bin`):

```bash
pio run -e supermini
pio run -t buildfs -e supermini   # LittleFS image (web/ assets)
pio run -t merge -e supermini
```

The merged image bundles bootloader + partitions + app (`app0`) + the LittleFS web assets, so a single flash gives a fully working device. `app1` is left blank for the first OTA update.

Put the board in download mode (hold **BOOT**, tap **RESET**), then flash with Chrome/Edge over USB.

### Full wipe (erase everything)

A complete erase clears **all** persisted state — Wi‑Fi credentials, saved locations, units, and range presets (NVS), both app slots, and the LittleFS web assets — for a clean factory state. A full erase is **required** after changing the partition table (e.g. upgrading from the old single-app layout to the dual-OTA layout).

Put the board in download mode first (hold **BOOT**, tap **RESET**).

**esptool (USB):**

```bash
esptool.py --chip esp32c3 erase_flash
esptool.py --chip esp32c3 --baud 921600 write_flash 0x0 release/plane-radar-merged.bin
```

**PlatformIO** (builds + uploads firmware and web assets separately):

```bash
pio run -t erase -e supermini          # wipe the whole chip
pio run -t upload -e supermini         # firmware
pio run -t uploadfs -e supermini       # web UI assets (LittleFS)
```

**Web flasher** ([esptool-js](https://espressif.github.io/esptool-js/)): connect, click **Erase Flash**, then flash `plane-radar-merged.bin` at **0x0**.

> A plain reflash over the top **keeps** NVS, so your Wi‑Fi and saved locations survive. Only `erase_flash` / **Erase Flash** wipes everything. For a Wi‑Fi-only reset (keeping locations/units), use the Device page button or hold **BOOT** 3 s — no reflash needed.

### CI and releases (GitHub Actions)

| Workflow | When | Output |
| --- | --- | --- |
| [Build](.github/workflows/build.yml) | Push / PR to `main` | Artifact `plane-radar-supermini` (merged + split `.bin` files, ~90 days) |
| [Release](.github/workflows/release.yml) | Git tag `v*` (e.g. `v1.0.0`) | GitHub Release asset `plane-radar-v1.0.0.bin` + `.sha256` |

To ship a version users can download:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The release workflow builds firmware in CI and attaches the merged image to the release. Download from **Releases** on GitHub, then flash at **0x0** (ESP32-C3, 4 MB).

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
