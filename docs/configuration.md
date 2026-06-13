# Configuration & project layout

[← Back to README](../README.md)

## Configuration

Edit **[`include/config.h`](../include/config.h)** for hardware and behavior:

| Area | Keys / notes |
| --- | --- |
| Portal / hostname | `kPortalApName`, `kPortalIp`, `kPortalHostname` (single hostname for the setup AP **and** the LAN/mDNS in STA mode) / `kPortalHostUrl`; needs `-DWM_MDNS` in `platformio.ini` |
| Wi‑Fi timing | connect attempts, reconnect grace, portal timeout (`0` = no timeout) |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| Display SPI | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |

Default range presets: [`include/ui/radar_range.h`](../include/ui/radar_range.h) (`kDefaultRangePresets`). These are compile-time fallbacks; runtime values are configurable via the portal and stored in NVS.

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
