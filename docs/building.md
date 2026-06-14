# Building & flashing

[← Back to README](../README.md)

> **Just want to flash?** See the [step-by-step guide in the README](../README.md#-flashing-the-firmware) — no build tools needed.

## Build from source (PlatformIO)

```bash
pio run -e supermini -t upload          # firmware
pio run -e supermini -t uploadfs        # web UI assets (LittleFS, from web/)
pio device monitor
```

- PlatformIO env: **`supermini`**
- Serial: **115200** baud
- USB CDC on boot enabled in `platformio.ini` for the Super Mini
- The web UI (`web/`) is flashed to the **LittleFS** partition via `uploadfs`; edit files in `web/` and re-run `uploadfs` to update them without rebuilding firmware
- Flash layout (`partitions/plane_radar.csv`): single 3 MB app slot (`factory`) + 960 KB LittleFS for web assets

## Debug build

A second environment enables verbose render-pipeline logging to the serial monitor:

```bash
pio run -e supermini-debug -t upload
pio device monitor
```

This adds `-DRADAR_DISPLAY_DEBUG` and prints per-frame checkpoints (`renderFrame start`, `grid done`, `pushSprite done`, etc.) without affecting the normal `supermini` build.

## Building the merged image

Single `.bin` for [ESPWebTool](https://esptool.spacehuhn.com/) and similar tools (ESP32-C3, 4 MB, flash at **0x0**):

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

The merged image bundles bootloader + partitions + app + LittleFS web assets, so a single flash gives a fully working device.

Put the board in download mode (unplug, hold **BOOT**, replug, release **BOOT**), then flash with Chrome/Edge over USB.

<details>
<summary><strong>Full wipe (erase everything)</strong></summary>

A complete erase clears **all** persisted state — Wi‑Fi credentials, saved locations, units, and range presets (NVS), the app slot, and the LittleFS web assets — for a clean factory state. A full erase is **required** after changing the partition table.

Put the board in download mode first (unplug, hold **BOOT**, replug, release **BOOT**).

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

**Web flasher** ([ESPWebTool](https://esptool.spacehuhn.com/)): connect, click **Erase**, then flash `plane-radar-merged.bin` at **0x0**.

> A plain reflash over the top **keeps** NVS, so your Wi‑Fi and saved locations survive. Only `erase_flash` / **Erase** wipes everything. For a Wi‑Fi-only reset (keeping locations/units), use the Device page button or hold **BOOT** 3 s — no reflash needed.

</details>

<details>
<summary><strong>CI and releases (GitHub Actions)</strong></summary>

| Workflow | When | Output |
| --- | --- | --- |
| [Build](../.github/workflows/build.yml) | Push / PR to `main` | Artifact `plane-radar-supermini` (merged + split `.bin` files, ~90 days) |
| [Release](../.github/workflows/release.yml) | Git tag `x.x.x` (e.g. `1.0.0`) | GitHub Release asset `plane-radar-1.0.0.bin` + `.sha256` |

To ship a version users can download:

```bash
git tag 1.0.0
git push origin 1.0.0
```

The release workflow builds firmware in CI and attaches the merged image to the release. Download from **Releases** on GitHub, then flash at **0x0** (ESP32-C3, 4 MB).

</details>
