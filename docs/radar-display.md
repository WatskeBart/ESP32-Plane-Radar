# Radar display

[← Back to README](../README.md)

## Grid

- Dark blue background, subdued green rings and crosshairs
- White **N / S / E / W** at the bezel; range label on the **east** spoke (ring 3 = ¾ of outer radius)
- White center dot

Layout and colors: [`include/ui/radar_theme.h`](../include/ui/radar_theme.h).

## Range presets

Four range presets are configurable via the portal (`/radar` → Range presets). The default values are:

| Ring 3 label | Outer radius (aircraft scale) |
| --- | --- |
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Presets and miles/km choice persist across reboot (`planeradar` NVS namespace). Tap BOOT to cycle presets on-device.

## Runways

- Major airports from OurAirports (`large_airport`); all open runway strips in range (helipads excluded)
- Teal runway lines with one ICAO label per airport (e.g. `KJFK`); toggle in the Wi‑Fi setup portal
- Update the embedded list: `python3 scripts/build_large_airports.py`

## Aircraft

- **Inside the outer ring** — red heading triangle, magenta speed vector (clipped at the ring), callsign / type / altitude tags
- **Outside the ring** (still within ADS-B fetch) — small **red dot on the screen rim** at the correct bearing (direction cue; not distance-accurate past the ring)
- **Tags** — placed toward the **center**: west (left) → tag on the **right** of the symbol; east (right) → tag on the **left**; each of the three lines can be hidden individually via the portal
- **Watch filter** — when prefixes are configured, only aircraft whose callsign starts with a matching prefix are shown; all others are hidden

As range decreases (or aircraft approach), targets move inward; beyond-ring dots become full symbols when they cross the outer ring.

## Aircraft count overlay

When enabled via the portal, the number of visible aircraft is shown in bright green at the bottom of the radar disc. When a watch filter is active, the count reflects only the matching aircraft. Toggle with the **Show aircraft count on display** checkbox.

## Stale data indicator

A small dot in the top-right corner (between the outer ring and the screen edge) shows API health:

| Color | Meaning |
| --- | --- |
| None (hidden) | Data is fresh (< 10 s old) |
| Orange | Last successful fetch was 10–30 s ago |
| Red | Last successful fetch was > 30 s ago or never |

## ADS-B

- Source: `https://opendata.adsb.fi/api/v3/`
- Fetch radius: `ui::radar::fetchRadiusKm()` — scales with the active preset to roughly the screen edge (so rim dots have data)
- Poll interval: [`kAdsbFetchIntervalMs`](../include/config.h#L55) (3 s)
- Ground aircraft hidden by default [`kAdsbShowGroundAircraft`](../include/config.h#L59)
