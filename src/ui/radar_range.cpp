#include "ui/radar_range.h"

#include "ui/radar_theme.h"

#include <Preferences.h>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace ui::radar {

namespace {

constexpr char kPrefsNamespace[] = "planeradar";
constexpr char kPrefsRangeKey[] = "rangeIdx";
constexpr char kPrefsMilesKey[] = "useMiles";
constexpr char kPrefsRunwaysKey[] = "showRwys";
constexpr char kPrefsCountKey[] = "showCount";
constexpr char kPrefsWatchKey[] = "watchSign";
constexpr char kPrefsShowFlightKey[] = "showFlight";
constexpr char kPrefsShowTypeKey[] = "showAcType";
constexpr char kPrefsShowAltKey[] = "showAlt";
constexpr char kPrefsUseMetersKey[] = "useMeters";
constexpr char kPrefsPreset0Key[] = "p0";
constexpr char kPrefsPreset1Key[] = "p1";
constexpr char kPrefsPreset2Key[] = "p2";
constexpr char kPrefsPreset3Key[] = "p3";
constexpr uint8_t kDefaultRangeIndex = 1;  // 10 km ring
constexpr float kKmPerMile = 1.609344f;
constexpr float kPresetMinKm = 0.5f;
constexpr float kPresetMaxKm = 500.0f;

Preferences s_prefs;
uint8_t s_range_index = kDefaultRangeIndex;
bool s_use_miles = false;
bool s_show_runways = true;
bool s_show_aircraft_count = false;
char s_watch_callsign[49] = "";  // comma-separated prefixes, e.g. "KLM,BAW,DLH"
bool s_show_tag_callsign = true;
bool s_show_tag_type = true;
bool s_show_tag_alt = true;
bool s_use_meters = false;

RangePreset s_range_presets[kRangePresetCount] = {
    kDefaultRangePresets[0],
    kDefaultRangePresets[1],
    kDefaultRangePresets[2],
    kDefaultRangePresets[3],
};

void saveRangeIndex() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putUChar(kPrefsRangeKey, s_range_index);
  s_prefs.end();
}

void saveUseMiles() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putBool(kPrefsMilesKey, s_use_miles);
  s_prefs.end();
}

void saveShowRunways() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putBool(kPrefsRunwaysKey, s_show_runways);
  s_prefs.end();
}

void saveShowAircraftCount() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putBool(kPrefsCountKey, s_show_aircraft_count);
  s_prefs.end();
}

void saveWatchCallsign() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putString(kPrefsWatchKey, s_watch_callsign);
  s_prefs.end();
}

void saveTagCallsign() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putBool(kPrefsShowFlightKey, s_show_tag_callsign);
  s_prefs.end();
}

void saveTagType() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putBool(kPrefsShowTypeKey, s_show_tag_type);
  s_prefs.end();
}

void saveTagAlt() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putBool(kPrefsShowAltKey, s_show_tag_alt);
  s_prefs.end();
}

void saveUseMeters() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putBool(kPrefsUseMetersKey, s_use_meters);
  s_prefs.end();
}

void saveRangePresets() {
  if (!s_prefs.begin(kPrefsNamespace, false)) return;
  s_prefs.putFloat(kPrefsPreset0Key, s_range_presets[0].ring3_km);
  s_prefs.putFloat(kPrefsPreset1Key, s_range_presets[1].ring3_km);
  s_prefs.putFloat(kPrefsPreset2Key, s_range_presets[2].ring3_km);
  s_prefs.putFloat(kPrefsPreset3Key, s_range_presets[3].ring3_km);
  s_prefs.end();
}

bool portalCheckboxChecked(const char* value) {
  if (value == nullptr || value[0] == '\0') return false;
  // WiFiManager checkbox: value="T"/"F" (single char) or standard "on".
  if ((value[0] == 'T' || value[0] == 't' || value[0] == 'F' || value[0] == 'f') &&
      value[1] == '\0') {
    return true;
  }
  return strcmp(value, "on") == 0;
}

}  // namespace

void rangeInit() {
  if (!s_prefs.begin(kPrefsNamespace, true)) return;

  const uint8_t saved = s_prefs.getUChar(kPrefsRangeKey, kDefaultRangeIndex);
  s_range_index = (saved < kRangePresetCount) ? saved : kDefaultRangeIndex;
  s_use_miles = s_prefs.getBool(kPrefsMilesKey, false);
  s_show_runways = s_prefs.getBool(kPrefsRunwaysKey, true);
  s_show_aircraft_count = s_prefs.getBool(kPrefsCountKey, false);
  s_prefs.getString(kPrefsWatchKey, s_watch_callsign, sizeof(s_watch_callsign));
  s_show_tag_callsign = s_prefs.getBool(kPrefsShowFlightKey, true);
  s_show_tag_type     = s_prefs.getBool(kPrefsShowTypeKey, true);
  s_show_tag_alt      = s_prefs.getBool(kPrefsShowAltKey, true);
  s_use_meters        = s_prefs.getBool(kPrefsUseMetersKey, false);

  const char* pkeys[kRangePresetCount] = {
      kPrefsPreset0Key, kPrefsPreset1Key, kPrefsPreset2Key, kPrefsPreset3Key,
  };
  for (size_t i = 0; i < kRangePresetCount; ++i) {
    if (s_prefs.isKey(pkeys[i])) {
      const float v = s_prefs.getFloat(pkeys[i], kDefaultRangePresets[i].ring3_km);
      if (v >= kPresetMinKm && v <= kPresetMaxKm) {
        s_range_presets[i].ring3_km = v;
        s_range_presets[i].outer_km = v * kRing3ToOuterKm;
      }
    }
  }

  s_prefs.end();
}

void rangeNext() {
  s_range_index = static_cast<uint8_t>((s_range_index + 1) % kRangePresetCount);
  saveRangeIndex();
}

const RangePreset& rangeCurrent() { return s_range_presets[s_range_index]; }

const RangePreset* rangePresets() { return s_range_presets; }

uint8_t rangeIndex() { return s_range_index; }

float fetchRadiusKm() {
  const float outer_km = rangeCurrent().outer_km;
  const float screen_r_px =
      static_cast<float>(kCenterX - kBeyondRingScreenMarginPx);
  return outer_km * (screen_r_px / static_cast<float>(kGridOuterRadius));
}

bool useMiles() { return s_use_miles; }
bool showRunways() { return s_show_runways; }
bool showAircraftCount() { return s_show_aircraft_count; }
bool showTagCallsign() { return s_show_tag_callsign; }
bool showTagType() { return s_show_tag_type; }
bool showTagAlt() { return s_show_tag_alt; }
bool useMeters() { return s_use_meters; }
const char* watchCallsign() { return s_watch_callsign; }

void saveMilesFromPortal(const char* checkbox_value) {
  s_use_miles = portalCheckboxChecked(checkbox_value);
  saveUseMiles();
  Serial.printf("Distance units: %s\n", s_use_miles ? "miles" : "km");
}

void saveRunwaysFromPortal(const char* checkbox_value) {
  s_show_runways = portalCheckboxChecked(checkbox_value);
  saveShowRunways();
  Serial.printf("Runway overlay: %s\n", s_show_runways ? "on" : "off");
}

void saveAircraftCountFromPortal(const char* checkbox_value) {
  s_show_aircraft_count = portalCheckboxChecked(checkbox_value);
  saveShowAircraftCount();
  Serial.printf("Aircraft count: %s\n", s_show_aircraft_count ? "on" : "off");
}

void saveTagCallsignFromPortal(const char* checkbox_value) {
  s_show_tag_callsign = portalCheckboxChecked(checkbox_value);
  saveTagCallsign();
}

void saveTagTypeFromPortal(const char* checkbox_value) {
  s_show_tag_type = portalCheckboxChecked(checkbox_value);
  saveTagType();
}

void saveTagAltFromPortal(const char* checkbox_value) {
  s_show_tag_alt = portalCheckboxChecked(checkbox_value);
  saveTagAlt();
}

void saveUseMetersFromPortal(const char* checkbox_value) {
  s_use_meters = portalCheckboxChecked(checkbox_value);
  saveUseMeters();
}

void saveRangePresetsFromPortal(const char* r0, const char* r1, const char* r2,
                                const char* r3) {
  const char* strs[kRangePresetCount] = {r0, r1, r2, r3};
  float vals[kRangePresetCount];

  for (size_t i = 0; i < kRangePresetCount; ++i) {
    if (strs[i] == nullptr || strs[i][0] == '\0') return;
    char* end = nullptr;
    vals[i] = strtof(strs[i], &end);
    if (end == strs[i] || vals[i] < kPresetMinKm || vals[i] > kPresetMaxKm) return;
  }
  for (size_t i = 1; i < kRangePresetCount; ++i) {
    if (vals[i] <= vals[i - 1]) return;
  }

  for (size_t i = 0; i < kRangePresetCount; ++i) {
    s_range_presets[i].ring3_km = vals[i];
    s_range_presets[i].outer_km = vals[i] * kRing3ToOuterKm;
  }
  saveRangePresets();
  Serial.printf("Range presets: %.1f/%.1f/%.1f/%.1f km\n",
                vals[0], vals[1], vals[2], vals[3]);
}

void saveWatchCallsignFromPortal(const char* value) {
  if (value == nullptr) value = "";
  // Strip spaces; uppercase letters/digits; keep commas; no leading/trailing/double commas.
  char buf[sizeof(s_watch_callsign)];
  size_t out = 0;
  for (size_t i = 0; value[i] != '\0' && out < sizeof(buf) - 1; ++i) {
    const unsigned char c = static_cast<unsigned char>(value[i]);
    if (c == ' ') continue;
    if (c == ',') {
      if (out > 0 && buf[out - 1] != ',') buf[out++] = ',';
    } else if (isalnum(c) || c == '-') {
      buf[out++] = static_cast<char>(toupper(c));
    }
  }
  if (out > 0 && buf[out - 1] == ',') --out;
  buf[out] = '\0';
  memcpy(s_watch_callsign, buf, out + 1);
  saveWatchCallsign();
  Serial.printf("Watch prefixes: '%s'\n", s_watch_callsign);
}

void formatRing3Label(char* buf, size_t len, float ring3_km, bool use_miles) {
  if (use_miles) {
    const int mi = static_cast<int>(lroundf(ring3_km / kKmPerMile));
    snprintf(buf, len, "%dmi", mi);
  } else {
    const int km = static_cast<int>(lroundf(ring3_km));
    snprintf(buf, len, "%dkm", km);
  }
}

void formatCurrentRing3Label(char* buf, size_t len) {
  formatRing3Label(buf, len, rangeCurrent().ring3_km, s_use_miles);
}

void unitsReset() {
  s_use_miles = false;
  s_show_runways = true;
  if (s_prefs.begin(kPrefsNamespace, false)) {
    s_prefs.remove(kPrefsMilesKey);
    s_prefs.remove(kPrefsRunwaysKey);
    s_prefs.end();
  }
}

}  // namespace ui::radar
