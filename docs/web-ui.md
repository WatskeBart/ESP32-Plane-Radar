# Web interface

[← Back to README](../README.md)

The device runs **two different web UIs depending on its mode**:

| Mode | Server | URL | Purpose |
| --- | --- | --- | --- |
| **Setup (AP)** | WiFiManager | `http://plane-radar.local` / `http://192.168.4.1` | Join your Wi‑Fi (only when there are no/invalid credentials, or after a reset) |
| **Connected (STA)** | Built-in radar UI (LittleFS) | `http://plane-radar.local` / `http://<device-ip>` | Menu → Configure Radar, Locations, Device |

Once connected to your network, WiFiManager's pages are no longer served — only the custom radar UI is reachable.

## Setup (AP) mode

1. Connect to the **`PlaneRadar-Setup`** Wi‑Fi AP
2. Open **`http://plane-radar.local`** or **`http://192.168.4.1`** (both shown on the yellow setup screen; the captive portal may open automatically)
3. Select your home Wi‑Fi and save — the device reboots and connects

Or hardcode fallback AP credentials in [config.h](../include/config.h#L24).

## Connected (STA) mode

Browse to **`http://plane-radar.local`** — or the device IP from your router. The hostname is `kPortalHostname` in [config.h](../include/config.h#L21), the single name used for the setup AP, DHCP, and mDNS. Some clients resolve `.local` slowly — use the IP if needed. If you run more than one device on the same LAN, give each a distinct `kPortalHostname` to avoid a name clash.

The root path (`/`) is a **menu** with three buttons; each page has a **← Back to menu** link.

Pages: **Configure Radar** (`/radar`), **Locations** (`/locs`), **Device** (`/device`).

### Configure Radar (`/radar`)

Saving returns to this page (not the main menu), so settings can be tweaked without navigating back.

<details>
<summary><strong>All radar settings</strong></summary>

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

</details>

### Locations (`/locs`)

Save up to **3 named locations** in flash. Use the **Use** button to switch the radar center to a saved location; **Delete** to remove it. Adding a new manual lat/lon on the main radar page clears the active saved location.

### Device (`/device`)

The **Info** panel shows the running **firmware version** (git tag/commit, injected at build by [scripts/git_version.py](../scripts/git_version.py)), hostname, mDNS name, IP, and connected network.

| Action | Effect |
| --- | --- |
| **Restart device** | Soft reboot |
| **Reset WiFi credentials** | Clears **only** the saved Wi‑Fi (keeps locations, units, presets) and reboots into setup AP mode |

The BOOT **hold 3 s** does a fuller reset (Wi‑Fi + location + units), whereas the Device page's reset is Wi‑Fi-only.

After a reset, the device reboots and shows the setup screen immediately (no "Connecting" loop on stale credentials).

To update firmware, flash a new merged image via [ESPWebTool](https://esptool.spacehuhn.com/) — see [Building & flashing](building.md).
