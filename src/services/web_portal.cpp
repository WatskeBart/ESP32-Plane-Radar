#include "services/web_portal.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_system.h>

#include <cstdio>
#include <cstring>

#include "config.h"
#include "services/radar_location.h"
#include "services/wifi_setup.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"

namespace services::web {

namespace {

WebServer s_server(80);
bool s_mounted = false;
bool s_active = false;
bool s_routes_registered = false;

const char* contentType(const char* path) {
  const size_t len = strlen(path);
  auto ends = [&](const char* ext) {
    const size_t el = strlen(ext);
    return len >= el && strcmp(path + len - el, ext) == 0;
  };
  if (ends(".html")) return "text/html";
  if (ends(".css")) return "text/css";
  if (ends(".js")) return "application/javascript";
  if (ends(".json")) return "application/json";
  return "text/plain";
}

void serveFile(const char* path) {
  if (!s_mounted || !LittleFS.exists(path)) {
    s_server.send(404, "text/plain", "Not found");
    return;
  }
  File f = LittleFS.open(path, "r");
  if (!f) {
    s_server.send(500, "text/plain", "Read error");
    return;
  }
  s_server.streamFile(f, contentType(path));
  f.close();
}

/** Maps a JSON boolean onto the "on"/"" strings the portal save fns expect. */
const char* checkboxArg(const JsonDocument& doc, const char* key) {
  return doc[key].as<bool>() ? "on" : "";
}

void handleGetRadar() {
  JsonDocument doc;
  doc["lat"] = services::location::lat();
  doc["lon"] = services::location::lon();
  doc["useMiles"] = ui::radar::useMiles();
  doc["showRunways"] = ui::radar::showRunways();
  doc["showCount"] = ui::radar::showAircraftCount();
  doc["showFlight"] = ui::radar::showTagCallsign();
  doc["showType"] = ui::radar::showTagType();
  doc["showAlt"] = ui::radar::showTagAlt();
  doc["useMeters"] = ui::radar::useMeters();
  doc["watch"] = ui::radar::watchCallsign();

  JsonArray presets = doc["presets"].to<JsonArray>();
  const ui::radar::RangePreset* p = ui::radar::rangePresets();
  for (size_t i = 0; i < ui::radar::kRangePresetCount; ++i) {
    presets.add(p[i].ring3_km);
  }

  String out;
  serializeJson(doc, out);
  s_server.send(200, "application/json", out);
}

void handlePostRadar() {
  JsonDocument doc;
  if (deserializeJson(doc, s_server.arg("plain"))) {
    s_server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }

  char lat_buf[24];
  char lon_buf[24];
  snprintf(lat_buf, sizeof(lat_buf), "%.6f", doc["lat"].as<double>());
  snprintf(lon_buf, sizeof(lon_buf), "%.6f", doc["lon"].as<double>());
  if (!services::location::saveFromStrings(lat_buf, lon_buf)) {
    Serial.println("Invalid lat/lon in API — keeping previous location");
  }

  ui::radar::saveMilesFromPortal(checkboxArg(doc, "useMiles"));
  ui::radar::saveRunwaysFromPortal(checkboxArg(doc, "showRunways"));
  ui::radar::saveAircraftCountFromPortal(checkboxArg(doc, "showCount"));
  ui::radar::saveTagCallsignFromPortal(checkboxArg(doc, "showFlight"));
  ui::radar::saveTagTypeFromPortal(checkboxArg(doc, "showType"));
  ui::radar::saveTagAltFromPortal(checkboxArg(doc, "showAlt"));
  ui::radar::saveUseMetersFromPortal(checkboxArg(doc, "useMeters"));
  ui::radar::saveWatchCallsignFromPortal(doc["watch"] | "");

  static_assert(ui::radar::kRangePresetCount == 4,
                "radar API assumes 4 range presets");
  char preset_buf[4][16];
  JsonArrayConst presets = doc["presets"];
  for (size_t i = 0; i < 4; ++i) {
    snprintf(preset_buf[i], sizeof(preset_buf[i]), "%.3f", presets[i].as<float>());
  }
  ui::radar::saveRangePresetsFromPortal(preset_buf[0], preset_buf[1],
                                        preset_buf[2], preset_buf[3]);

  ui::radarDisplayInvalidateLabelMetrics();
  s_server.send(200, "application/json", "{\"ok\":true}");
}

void handleGetLocations() {
  JsonDocument doc;
  doc["max"] = static_cast<int>(services::location::kMaxSavedLocations);
  const uint8_t active = services::location::activeLocationIndex();
  doc["active"] =
      active == services::location::kNoActiveSlot ? -1 : static_cast<int>(active);

  JsonArray arr = doc["locations"].to<JsonArray>();
  const services::location::SavedLocation* locs =
      services::location::savedLocations();
  for (uint8_t i = 0; i < services::location::kMaxSavedLocations; ++i) {
    JsonObject o = arr.add<JsonObject>();
    o["slot"] = i;
    o["valid"] = locs[i].valid;
    o["name"] = locs[i].name;
    o["lat"] = locs[i].lat;
    o["lon"] = locs[i].lon;
    o["active"] = (active == i);
  }

  String out;
  serializeJson(doc, out);
  s_server.send(200, "application/json", out);
}

void handlePostLocations() {
  JsonDocument doc;
  if (deserializeJson(doc, s_server.arg("plain"))) {
    s_server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }

  const char* action = doc["action"] | "";
  const uint8_t slot = doc["slot"].as<uint8_t>();

  if (strcmp(action, "use") == 0) {
    if (!services::location::setActiveLocation(slot)) {
      Serial.printf("Failed to set active location slot %u\n", slot);
    }
  } else if (strcmp(action, "delete") == 0) {
    services::location::deleteLocation(slot);
  } else if (strcmp(action, "save") == 0) {
    char lat_buf[24];
    char lon_buf[24];
    snprintf(lat_buf, sizeof(lat_buf), "%.6f", doc["lat"].as<double>());
    snprintf(lon_buf, sizeof(lon_buf), "%.6f", doc["lon"].as<double>());
    if (!services::location::saveLocation(slot, doc["name"] | "", lat_buf, lon_buf)) {
      Serial.println("Invalid location data in API — slot not saved");
    }
  } else {
    s_server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad action\"}");
    return;
  }

  s_server.send(200, "application/json", "{\"ok\":true}");
}

void handleGetDevice() {
  JsonDocument doc;
  doc["version"] = config::kFirmwareVersion;
  doc["hostname"] = config::kPortalHostname;
  doc["mdns"] = String(config::kPortalHostname) + ".local";
  doc["ip"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  String out;
  serializeJson(doc, out);
  s_server.send(200, "application/json", out);
}

void handleRestart() {
  s_server.send(200, "application/json", "{\"ok\":true}");
  delay(300);
  esp_restart();
}

void handleWifiReset() {
  s_server.send(200, "application/json", "{\"ok\":true}");
  delay(300);
  wifiEraseCredentialsAndReboot();  // erases WiFi creds, then reboots to AP setup
}

// --- OTA firmware update (writes to the inactive app slot, then reboots) ---

void handleUpdateUpload() {
  HTTPUpload& up = s_server.upload();
  if (up.status == UPLOAD_FILE_START) {
    Serial.printf("OTA start: %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (Update.write(up.buf, up.currentSize) != up.currentSize) {
      Update.printError(Serial);
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA done: %u bytes\n", up.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handleUpdateDone() {
  const bool ok = !Update.hasError();
  s_server.sendHeader("Connection", "close");
  s_server.send(ok ? 200 : 500, "application/json",
                ok ? "{\"ok\":true}" : "{\"ok\":false}");
  if (ok) {
    delay(500);
    esp_restart();
  }
}

void registerRoutes() {
  // Pages.
  s_server.on("/", HTTP_GET, [] { serveFile("/index.html"); });
  s_server.on("/radar", HTTP_GET, [] { serveFile("/radar.html"); });
  s_server.on("/locs", HTTP_GET, [] { serveFile("/locs.html"); });
  s_server.on("/device", HTTP_GET, [] { serveFile("/device.html"); });
  s_server.on("/radar.css", HTTP_GET, [] { serveFile("/radar.css"); });
  s_server.on("/radar.js", HTTP_GET, [] { serveFile("/radar.js"); });
  s_server.on("/locs.js", HTTP_GET, [] { serveFile("/locs.js"); });
  s_server.on("/device.js", HTTP_GET, [] { serveFile("/device.js"); });

  // JSON API.
  s_server.on("/api/radar", HTTP_GET, handleGetRadar);
  s_server.on("/api/radar", HTTP_POST, handlePostRadar);
  s_server.on("/api/locations", HTTP_GET, handleGetLocations);
  s_server.on("/api/locations", HTTP_POST, handlePostLocations);
  s_server.on("/api/device", HTTP_GET, handleGetDevice);
  s_server.on("/api/restart", HTTP_POST, handleRestart);
  s_server.on("/api/wifi/reset", HTTP_POST, handleWifiReset);

  // OTA firmware upload.
  s_server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);

  s_server.onNotFound([] { s_server.send(404, "text/plain", "Not found"); });
}

}  // namespace

bool begin() {
  if (s_mounted) {
    return true;
  }
  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount failed — radar web assets unavailable");
    return false;
  }
  s_mounted = true;
  Serial.println("LittleFS mounted — radar web assets ready");
  return true;
}

void startStaServer() {
  if (s_active) {
    return;
  }
  if (!s_routes_registered) {
    registerRoutes();
    s_routes_registered = true;
  }
  s_server.begin();
  s_active = true;
}

void stopStaServer() {
  if (!s_active) {
    return;
  }
  s_server.stop();
  s_active = false;
}

void handleClient() {
  if (s_active) {
    s_server.handleClient();
  }
}

bool serverActive() { return s_active; }

}  // namespace services::web
