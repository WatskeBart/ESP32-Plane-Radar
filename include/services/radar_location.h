#pragma once

#include <cstddef>
#include <cstdint>

namespace services::location {

constexpr size_t kMaxSavedLocations = 3;
constexpr uint8_t kNoActiveSlot = 0xFF;

struct SavedLocation {
  char name[13];  // max 12 chars + null terminator
  double lat;
  double lon;
  bool valid;
};

/** Load saved lat/lon and location slots from NVS. Call once before WiFi setup. */
void init();

/** Active radar lat/lon (either a saved slot or custom coords). */
double lat();
double lon();

/** Parse portal strings, validate, persist to NVS, update runtime values.
 *  Resets the active slot index to kNoActiveSlot (custom coords). */
bool saveFromStrings(const char* lat_str, const char* lon_str);

/** Clear stored coordinates (e.g. with WiFi credential reset). */
void clear();

// --- Saved location slots ---

size_t savedLocationCount();
const SavedLocation* savedLocations();

/** Index of the active saved slot, or kNoActiveSlot if custom coords are in use. */
uint8_t activeLocationIndex();

/** Save or overwrite a slot. Validates lat/lon; trims name to 12 chars. */
bool saveLocation(uint8_t slot, const char* name, const char* lat_str,
                  const char* lon_str);

/** Make the given slot's coords the active lat/lon. */
bool setActiveLocation(uint8_t slot);

/** Remove a slot (marks it invalid; remaining slots keep their indices). */
void deleteLocation(uint8_t slot);

}  // namespace services::location
