---
name: project-features
description: Features added to ESP32-Plane-Radar beyond the original implementation
metadata:
  type: project
---

The following features were added in one session (2026-06-12):

**Feature 1 – Configurable Range Presets**
- `kRangePresets` renamed to `kDefaultRangePresets` (compile-time fallback)
- `rangePresets()` now returns a runtime `s_range_presets[]` array loaded from NVS keys `p0`–`p3`
- Web portal (`/radar`) gains 4 number inputs (km, must be strictly increasing)
- `saveRangePresetsFromPortal()` validates and persists; `radarDisplayInvalidateLabelMetrics()` called after save

**Feature 2 – API Stale/Error Indicator**
- `services::adsb::lastFetchAgeMs()` tracks ms since last successful fetch (ULONG_MAX if never)
- `drawStaleIndicator()` draws a 4px dot at NE strip position (200, 40): orange ≥10s, red ≥30s, background otherwise

**Feature 3 – Aircraft Count Overlay (togglable)**
- `ui::radar::showAircraftCount()` — persisted NVS key `showCount`, default false
- `drawAircraftCount()` renders the count just inside the south outer ring at (120, 208)
- Web portal checkbox: "Show aircraft count on display"

**Feature 5 – Multiple Saved Locations (up to 3)**
- `services::location::SavedLocation` struct; slots stored in NVS namespace "radar" (`s0v`, `s0name`, `s0lat`, `s0lon`, etc.)
- New API: `savedLocations()`, `savedLocationCount()`, `activeLocationIndex()`, `saveLocation()`, `setActiveLocation()`, `deleteLocation()`
- Web portal page `/locs` (GET) + `/locssave` (POST) with Use/Delete per slot, Add form
- When `saveFromStrings()` is called (main lat/lon form), `slotAct` resets to `kNoActiveSlot` (0xFF)

**Feature 6 – Aircraft Highlight / Watch Callsign**
- `ui::radar::watchCallsign()` — NVS key `watchSign`, max 8 chars, always stored uppercase
- `matchesWatchCallsign()` does case-insensitive prefix match on `plane.callsign`
- Matched aircraft drawn in `kColorWatched` (yellow) instead of red; same color for speed vector

**Why:** User-requested features for improved usability. NVS keys are in existing namespaces (`planeradar` for range/display settings, `radar` for location). Build succeeds at 39.8% flash / 15.6% RAM.
