#pragma once

#include <cstddef>
#include <cstdint>

namespace ui::radar {

/**
 * Range presets (label on ring 3 = ¾ of outer radius).
 *
 * Recommended for ADS-B on a 1.28″ display:
 *   5 km  — pattern / very local (airfield vicinity)
 *  10 km  — default; neighborhood spotting
 *  15 km  — wider local area
 *  25 km  — metro / regional picture
 *
 * Outer radius (for aircraft math) is ring-3 distance ÷ 0.75.
 */
struct RangePreset {
  /** Distance shown on ring 3 (¾ of outer radius), always stored in km. */
  float ring3_km;
  float outer_km;
};

constexpr float kRing3ToOuterKm = 4.0f / 3.0f;

constexpr RangePreset kDefaultRangePresets[] = {
    {5.0f, 5.0f * kRing3ToOuterKm},
    {10.0f, 10.0f * kRing3ToOuterKm},
    {15.0f, 15.0f * kRing3ToOuterKm},
    {25.0f, 25.0f * kRing3ToOuterKm},
};

constexpr size_t kRangePresetCount =
    sizeof(kDefaultRangePresets) / sizeof(kDefaultRangePresets[0]);

/** Runtime range presets (loaded from NVS, falls back to kDefaultRangePresets). */
const RangePreset* rangePresets();

/** Load saved range and distance units from flash. Call once after boot. */
void rangeInit();
/** Cycle preset and save to flash. */
void rangeNext();
const RangePreset& rangeCurrent();
uint8_t rangeIndex();
/** ADSB fetch radius (km): scaled to screen edge so beyond-ring dots have data. */
float fetchRadiusKm();

bool useMiles();
bool showRunways();
bool showAircraftCount();
/** Tag item visibility. */
bool showTagCallsign();
bool showTagType();
bool showTagAlt();
/** Use metres instead of feet for altitude tags. */
bool useMeters();
/** WiFi portal checkbox: "T" = checked, empty = unchecked. */
void saveMilesFromPortal(const char* checkbox_value);
void saveRunwaysFromPortal(const char* checkbox_value);
void saveAircraftCountFromPortal(const char* checkbox_value);
void saveTagCallsignFromPortal(const char* checkbox_value);
void saveTagTypeFromPortal(const char* checkbox_value);
void saveTagAltFromPortal(const char* checkbox_value);
void saveUseMetersFromPortal(const char* checkbox_value);
/** Save custom ring3 km values (r0..r3 as decimal strings). Ignored if invalid or non-increasing. */
void saveRangePresetsFromPortal(const char* r0, const char* r1, const char* r2,
                                const char* r3);
/** Callsign prefix (or ICAO hex) to highlight; empty string = disabled. Always uppercase. */
const char* watchCallsign();
void saveWatchCallsignFromPortal(const char* value);
void formatRing3Label(char* buf, size_t len, float ring3_km, bool use_miles);
void formatCurrentRing3Label(char* buf, size_t len);
/** Reset distance units to km (e.g. with WiFi credential wipe). */
void unitsReset();

}  // namespace ui::radar
