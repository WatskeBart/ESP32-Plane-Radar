#include "services/radar_location.h"

#include <Preferences.h>
#include <cstdlib>
#include <cstring>

#include "config.h"

namespace services::location {

namespace {

constexpr char kPrefsNamespace[] = "radar";
constexpr char kKeyLat[] = "lat";
constexpr char kKeyLon[] = "lon";
constexpr char kKeyActiveSlot[] = "slotAct";

// Per-slot NVS keys (indexed by slot 0-2)
const char* kSlotValidKeys[kMaxSavedLocations] = {"s0v", "s1v", "s2v"};
const char* kSlotNameKeys[kMaxSavedLocations] = {"s0name", "s1name", "s2name"};
const char* kSlotLatKeys[kMaxSavedLocations] = {"s0lat", "s1lat", "s2lat"};
const char* kSlotLonKeys[kMaxSavedLocations] = {"s0lon", "s1lon", "s2lon"};

double s_lat = config::kDefaultRadarLat;
double s_lon = config::kDefaultRadarLon;
uint8_t s_active_slot = kNoActiveSlot;
SavedLocation s_locations[kMaxSavedLocations] = {};

bool parseCoord(const char* text, double* out) {
  if (text == nullptr || text[0] == '\0') return false;
  char* end = nullptr;
  const double v = strtod(text, &end);
  if (end == text || (end != nullptr && *end != '\0')) return false;
  *out = v;
  return true;
}

bool validLatLon(double lat, double lon) {
  return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

void persistActiveCoords(double lat, double lon) {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putDouble(kKeyLat, lat);
  prefs.putDouble(kKeyLon, lon);
  prefs.end();
  s_lat = lat;
  s_lon = lon;
}

void persistActiveSlot() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putUChar(kKeyActiveSlot, s_active_slot);
  prefs.end();
}

void persistSlot(uint8_t slot) {
  if (slot >= kMaxSavedLocations) return;
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putBool(kSlotValidKeys[slot], s_locations[slot].valid);
  if (s_locations[slot].valid) {
    prefs.putString(kSlotNameKeys[slot], s_locations[slot].name);
    prefs.putDouble(kSlotLatKeys[slot], s_locations[slot].lat);
    prefs.putDouble(kSlotLonKeys[slot], s_locations[slot].lon);
  }
  prefs.end();
}

}  // namespace

void init() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);

  if (prefs.isKey(kKeyLat) && prefs.isKey(kKeyLon)) {
    const double lat = prefs.getDouble(kKeyLat, config::kDefaultRadarLat);
    const double lon = prefs.getDouble(kKeyLon, config::kDefaultRadarLon);
    if (validLatLon(lat, lon)) {
      s_lat = lat;
      s_lon = lon;
    }
  }

  s_active_slot = prefs.getUChar(kKeyActiveSlot, kNoActiveSlot);

  for (uint8_t i = 0; i < kMaxSavedLocations; ++i) {
    s_locations[i].valid = prefs.getBool(kSlotValidKeys[i], false);
    if (s_locations[i].valid) {
      prefs.getString(kSlotNameKeys[i], s_locations[i].name,
                      sizeof(s_locations[i].name));
      s_locations[i].lat = prefs.getDouble(kSlotLatKeys[i], 0.0);
      s_locations[i].lon = prefs.getDouble(kSlotLonKeys[i], 0.0);
      if (!validLatLon(s_locations[i].lat, s_locations[i].lon)) {
        s_locations[i].valid = false;
      }
    }
  }

  prefs.end();
}

double lat() { return s_lat; }
double lon() { return s_lon; }

bool saveFromStrings(const char* lat_str, const char* lon_str) {
  double lat = 0.0;
  double lon = 0.0;
  if (!parseCoord(lat_str, &lat) || !parseCoord(lon_str, &lon)) return false;
  if (!validLatLon(lat, lon)) return false;
  persistActiveCoords(lat, lon);
  s_active_slot = kNoActiveSlot;
  persistActiveSlot();
  Serial.printf("Radar location saved: %.6f, %.6f\n", lat, lon);
  return true;
}

void clear() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.remove(kKeyLat);
  prefs.remove(kKeyLon);
  prefs.remove(kKeyActiveSlot);
  prefs.end();
  s_lat = config::kDefaultRadarLat;
  s_lon = config::kDefaultRadarLon;
  s_active_slot = kNoActiveSlot;
}

size_t savedLocationCount() {
  size_t n = 0;
  for (size_t i = 0; i < kMaxSavedLocations; ++i) {
    if (s_locations[i].valid) ++n;
  }
  return n;
}

const SavedLocation* savedLocations() { return s_locations; }

uint8_t activeLocationIndex() { return s_active_slot; }

bool saveLocation(uint8_t slot, const char* name, const char* lat_str,
                  const char* lon_str) {
  if (slot >= kMaxSavedLocations) return false;
  double lat = 0.0;
  double lon = 0.0;
  if (!parseCoord(lat_str, &lat) || !parseCoord(lon_str, &lon)) return false;
  if (!validLatLon(lat, lon)) return false;

  s_locations[slot].valid = true;
  s_locations[slot].lat = lat;
  s_locations[slot].lon = lon;

  const size_t name_len = strnlen(name, sizeof(s_locations[slot].name) - 1);
  memcpy(s_locations[slot].name, name, name_len);
  s_locations[slot].name[name_len] = '\0';

  persistSlot(slot);
  Serial.printf("Location slot %u saved: '%s' %.6f, %.6f\n", slot,
                s_locations[slot].name, lat, lon);
  return true;
}

bool setActiveLocation(uint8_t slot) {
  if (slot >= kMaxSavedLocations || !s_locations[slot].valid) return false;
  persistActiveCoords(s_locations[slot].lat, s_locations[slot].lon);
  s_active_slot = slot;
  persistActiveSlot();
  Serial.printf("Active location: slot %u '%s'\n", slot, s_locations[slot].name);
  return true;
}

void deleteLocation(uint8_t slot) {
  if (slot >= kMaxSavedLocations) return;
  s_locations[slot].valid = false;
  s_locations[slot].name[0] = '\0';
  if (s_active_slot == slot) {
    s_active_slot = kNoActiveSlot;
    persistActiveSlot();
  }
  persistSlot(slot);
  Serial.printf("Location slot %u deleted\n", slot);
}

}  // namespace services::location
