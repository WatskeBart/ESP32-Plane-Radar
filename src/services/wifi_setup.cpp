#include "services/wifi_setup.h"

#include <WiFi.h>
#include <WiFiManager.h>

#include <cstdio>

#include <Preferences.h>
#include <esp_system.h>
#include <esp_wifi.h>

#ifdef WM_MDNS
#include <ESPmDNS.h>
#endif

#include "config.h"
#include "services/radar_location.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/status_screens.h"

portMUX_TYPE s_boot_mux = portMUX_INITIALIZER_UNLOCKED;
volatile bool s_boot_tap_pending = false;
volatile bool s_boot_is_down = false;
volatile unsigned long s_boot_down_ms = 0;
bool s_long_press_handled = false;
bool s_boot_interrupt_attached = false;

void IRAM_ATTR onBootButtonIsr() {
  const bool down = digitalRead(config::kBootPin) == LOW;
  const unsigned long now = millis();
  portENTER_CRITICAL_ISR(&s_boot_mux);
  if (down) {
    s_boot_is_down = true;
    s_boot_down_ms = now;
  } else if (s_boot_is_down) {
    const unsigned long held = now - s_boot_down_ms;
    if (held >= config::kBootTapMinMs && held < config::kBootResetHoldMs) {
      s_boot_tap_pending = true;
    }
    s_boot_is_down = false;
  }
  portEXIT_CRITICAL_ISR(&s_boot_mux);
}

void initBootButton() {
  pinMode(config::kBootPin, INPUT_PULLUP);
  if (s_boot_interrupt_attached) {
    return;
  }
  attachInterrupt(digitalPinToInterrupt(static_cast<uint8_t>(config::kBootPin)),
                  onBootButtonIsr, CHANGE);
  s_boot_interrupt_attached = true;
}

namespace {

/** Separate from planeradar prefs (rangeInit) to avoid NVS handle conflicts. */
constexpr char kWifiPrefsNamespace[] = "wifi";
constexpr char kPrefsForcePortalKey[] = "portal";

bool s_force_config_portal = false;
WiFiManager s_wm;
bool s_wm_configured = false;

void ensureWifiManager();
void startLanWebPortal();
void stopLanWebPortal();
bool wifiLinkUp();

constexpr int kCoordParamLen = 20;

void handleRadarPage() {
  char lat_buf[kCoordParamLen + 1];
  char lon_buf[kCoordParamLen + 1];
  snprintf(lat_buf, sizeof(lat_buf), "%.6f", services::location::lat());
  snprintf(lon_buf, sizeof(lon_buf), "%.6f", services::location::lon());

  const ui::radar::RangePreset* presets = ui::radar::rangePresets();

  String page = FPSTR(HTTP_HEAD_START);
  page.replace("{v}", "Configure Radar");
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_SCRIPT);
  String head_end = FPSTR(HTTP_HEAD_END);
  head_end.replace("{c}", "");
  page += head_end;

  page += F("<h1>Configure Radar</h1>"
            "<form method='POST' action='/radarsave'>"
            "<label for='rlat'>Latitude (deg)</label>"
            "<input id='rlat' name='radar_lat' type='number' step='0.000001' value='");
  page += lat_buf;
  page += F("'><br/>"
            "<label for='rlon'>Longitude (deg)</label>"
            "<input id='rlon' name='radar_lon' type='number' step='0.000001' value='");
  page += lon_buf;
  page += F("'><br/><br/>"
            "<input type='checkbox' id='use_miles' name='use_miles'");
  if (ui::radar::useMiles()) page += F(" checked");
  page += F("> <label for='use_miles'>Display distances in miles</label><br/>"
            "<input type='checkbox' id='show_runways' name='show_runways'");
  if (ui::radar::showRunways()) page += F(" checked");
  page += F("> <label for='show_runways'>Show airport runways</label><br/>"
            "<input type='checkbox' id='show_count' name='show_count'");
  if (ui::radar::showAircraftCount()) page += F(" checked");
  page += F("> <label for='show_count'>Show aircraft count on display</label><br/><br/>"
            "<b>Aircraft tag items:</b><br/>"
            "<input type='checkbox' id='show_flight' name='show_flight'");
  if (ui::radar::showTagCallsign()) page += F(" checked");
  page += F("> <label for='show_flight'>Show flight number / callsign</label><br/>"
            "<input type='checkbox' id='show_ac_type' name='show_ac_type'");
  if (ui::radar::showTagType()) page += F(" checked");
  page += F("> <label for='show_ac_type'>Show aircraft type</label><br/>"
            "<input type='checkbox' id='show_alt' name='show_alt'");
  if (ui::radar::showTagAlt()) page += F(" checked");
  page += F("> <label for='show_alt'>Show altitude</label><br/>"
            "<input type='checkbox' id='use_meters' name='use_meters'");
  if (ui::radar::useMeters()) page += F(" checked");
  page += F("> <label for='use_meters'>Show altitude in metres (instead of feet)</label><br/><br/>"
            "<label for='watch_sign'>Watch callsign prefixes (comma-separated, e.g. KLM,BAW — empty = off)</label>"
            "<input id='watch_sign' name='watch_sign' type='text' maxlength='48' value='");
  page += ui::radar::watchCallsign();
  page += F("'><br/><br/>"
            "<b>Range presets (km, must be increasing):</b><br/>");

  const char* preset_labels[] = {"Preset 1", "Preset 2", "Preset 3", "Preset 4"};
  const char* preset_names[] = {"range_0", "range_1", "range_2", "range_3"};
  for (int i = 0; i < 4; ++i) {
    char val_buf[12];
    snprintf(val_buf, sizeof(val_buf), "%.1f", presets[i].ring3_km);
    page += F("<label>");
    page += preset_labels[i];
    page += F(": <input name='");
    page += preset_names[i];
    page += F("' type='number' step='0.1' min='0.5' max='500' value='");
    page += val_buf;
    page += F("'></label><br/>");
  }

  page += F("<br/><button type='submit'>Save</button></form>");
  page += FPSTR(HTTP_BACKBTN);
  page += FPSTR(HTTP_END);
  s_wm.server->send(200, "text/html", page);
}

void handleRadarSave() {
  const String lat_str = s_wm.server->arg("radar_lat");
  const String lon_str = s_wm.server->arg("radar_lon");
  if (!services::location::saveFromStrings(lat_str.c_str(), lon_str.c_str())) {
    Serial.println("Invalid lat/lon in portal — keeping previous location");
  }
  ui::radar::saveMilesFromPortal(s_wm.server->arg("use_miles").c_str());
  ui::radar::saveRunwaysFromPortal(s_wm.server->arg("show_runways").c_str());
  ui::radar::saveAircraftCountFromPortal(s_wm.server->arg("show_count").c_str());
  ui::radar::saveTagCallsignFromPortal(s_wm.server->arg("show_flight").c_str());
  ui::radar::saveTagTypeFromPortal(s_wm.server->arg("show_ac_type").c_str());
  ui::radar::saveTagAltFromPortal(s_wm.server->arg("show_alt").c_str());
  ui::radar::saveUseMetersFromPortal(s_wm.server->arg("use_meters").c_str());
  ui::radar::saveWatchCallsignFromPortal(s_wm.server->arg("watch_sign").c_str());
  ui::radar::saveRangePresetsFromPortal(
      s_wm.server->arg("range_0").c_str(), s_wm.server->arg("range_1").c_str(),
      s_wm.server->arg("range_2").c_str(), s_wm.server->arg("range_3").c_str());
  ui::radarDisplayInvalidateLabelMetrics();
  s_wm.server->sendHeader("Location", "/radar");
  s_wm.server->send(302, "text/plain", "");
}

void handleLocsPage() {
  const services::location::SavedLocation* locs = services::location::savedLocations();
  const uint8_t active_slot = services::location::activeLocationIndex();

  String page = FPSTR(HTTP_HEAD_START);
  page.replace("{v}", "Saved Locations");
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_SCRIPT);
  String head_end = FPSTR(HTTP_HEAD_END);
  head_end.replace("{c}", "");
  page += head_end;
  page += F("<h1>Saved Locations</h1>");

  bool any = false;
  for (uint8_t i = 0; i < services::location::kMaxSavedLocations; ++i) {
    if (!locs[i].valid) continue;
    any = true;
    char lat_buf[kCoordParamLen + 1];
    char lon_buf[kCoordParamLen + 1];
    snprintf(lat_buf, sizeof(lat_buf), "%.6f", locs[i].lat);
    snprintf(lon_buf, sizeof(lon_buf), "%.6f", locs[i].lon);

    page += F("<div style='border:1px solid #aaa;padding:6px;margin-bottom:6px'>");
    page += F("<b>");
    page += locs[i].name[0] ? locs[i].name : "(unnamed)";
    page += F("</b>");
    if (active_slot == i) page += F(" &#9679;");
    page += F("<br/><small>");
    page += lat_buf;
    page += F(", ");
    page += lon_buf;
    page += F("</small><br/>");

    // Use button
    page += F("<form style='display:inline' method='POST' action='/locssave'>"
              "<input type='hidden' name='action' value='use'>"
              "<input type='hidden' name='slot' value='");
    page += i;
    page += F("'><button>Use</button></form> ");

    // Delete button
    page += F("<form style='display:inline' method='POST' action='/locssave'>"
              "<input type='hidden' name='action' value='delete'>"
              "<input type='hidden' name='slot' value='");
    page += i;
    page += F("'><button>Delete</button></form>");
    page += F("</div>");
  }
  if (!any) {
    page += F("<p>No saved locations yet.</p>");
  }

  // Find next free slot
  int free_slot = -1;
  for (uint8_t i = 0; i < services::location::kMaxSavedLocations; ++i) {
    if (!locs[i].valid) { free_slot = i; break; }
  }

  if (free_slot >= 0) {
    page += F("<h2>Add Location</h2>"
              "<form method='POST' action='/locssave'>"
              "<input type='hidden' name='action' value='save'>"
              "<input type='hidden' name='slot' value='");
    page += free_slot;
    page += F("'>"
              "<label>Name <input name='name' maxlength='12' value=''></label><br/>"
              "<label>Latitude <input name='lat' type='number' step='0.000001'></label><br/>"
              "<label>Longitude <input name='lon' type='number' step='0.000001'></label><br/><br/>"
              "<button type='submit'>Save</button></form>");
  } else {
    page += F("<p><i>Maximum of 3 locations saved.</i></p>");
  }

  page += FPSTR(HTTP_BACKBTN);
  page += FPSTR(HTTP_END);
  s_wm.server->send(200, "text/html", page);
}

void handleLocsSave() {
  const String action = s_wm.server->arg("action");
  const String slot_str = s_wm.server->arg("slot");
  const uint8_t slot = static_cast<uint8_t>(slot_str.toInt());

  if (action == "use") {
    if (!services::location::setActiveLocation(slot)) {
      Serial.printf("Failed to set active location slot %u\n", slot);
    }
  } else if (action == "delete") {
    services::location::deleteLocation(slot);
  } else if (action == "save") {
    const String name = s_wm.server->arg("name");
    const String lat_str = s_wm.server->arg("lat");
    const String lon_str = s_wm.server->arg("lon");
    if (!services::location::saveLocation(slot, name.c_str(), lat_str.c_str(),
                                          lon_str.c_str())) {
      Serial.println("Invalid location data in portal — slot not saved");
    }
  }

  s_wm.server->sendHeader("Location", "/locs");
  s_wm.server->send(302, "text/plain", "");
}

void setupRadarRoutes() {
  s_wm.server->on("/radar", HTTP_GET, handleRadarPage);
  s_wm.server->on("/radarsave", HTTP_POST, handleRadarSave);
  s_wm.server->on("/locs", HTTP_GET, handleLocsPage);
  s_wm.server->on("/locssave", HTTP_POST, handleLocsSave);
}

void markForceConfigPortal() {
  s_force_config_portal = true;
  Preferences prefs;
  if (!prefs.begin(kWifiPrefsNamespace, false)) {
    return;
  }
  prefs.putBool(kPrefsForcePortalKey, true);
  prefs.end();
}

bool consumeForceConfigPortal() {
  if (s_force_config_portal) {
    s_force_config_portal = false;
    Preferences prefs;
    if (prefs.begin(kWifiPrefsNamespace, false)) {
      prefs.remove(kPrefsForcePortalKey);
      prefs.end();
    }
    return true;
  }

  Preferences prefs;
  if (!prefs.begin(kWifiPrefsNamespace, true)) {
    return false;
  }
  const bool pending = prefs.getBool(kPrefsForcePortalKey, false);
  prefs.end();
  if (!pending) {
    return false;
  }

  if (prefs.begin(kWifiPrefsNamespace, false)) {
    prefs.remove(kPrefsForcePortalKey);
    prefs.end();
  }
  return true;
}

bool storedWifiCredentials() {
  wifi_mode_t mode = WIFI_MODE_NULL;
  if (esp_wifi_get_mode(&mode) != ESP_OK || mode == WIFI_MODE_NULL) {
    WiFi.mode(WIFI_STA);
    delay(50);
  }

  wifi_config_t conf = {};
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) {
    return false;
  }
  return conf.sta.ssid[0] != '\0';
}

void eraseWifiCredentials() {
  stopLanWebPortal();
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_OFF);
  delay(100);

  ensureWifiManager();
  WiFi.persistent(true);
  s_wm.resetSettings();
  s_wm.erase();
  WiFi.disconnect(true, true);
  WiFi.persistent(false);

  WiFi.mode(WIFI_OFF);
  delay(100);
}

void resetWifiCredentials() {
  markForceConfigPortal();
  eraseWifiCredentials();
  services::location::clear();
  ui::radar::unitsReset();
  Serial.println("WiFi credentials, location, and units cleared");
}

void onConfigPortalApStarted(WiFiManager*) {
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  statusScreenPortal();
#ifdef WM_MDNS
  if (MDNS.begin(config::kPortalHostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Setup portal: http://%s.local (or http://%s)\n",
                  config::kPortalHostname, config::kPortalIp);
  } else {
    Serial.printf("Setup portal: http://%s (mDNS unavailable)\n", config::kPortalIp);
  }
#else
  Serial.printf("Setup portal: http://%s\n", config::kPortalIp);
#endif
}

bool wifiLinkUp() {
  return WiFi.status() == WL_CONNECTED &&
         WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

void ensureWifiManager() {
  if (s_wm_configured) {
    return;
  }
  s_wm.setConfigPortalTimeout(config::kWifiPortalTimeoutSec);
  s_wm.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                           IPAddress(255, 255, 255, 0));
  s_wm.setHostname(config::kPortalHostname);
  s_wm.setAPCallback(onConfigPortalApStarted);
  std::vector<const char*> menu = {"wifi", "info", "exit", "sep", "custom"};
  s_wm.setMenu(menu);
  s_wm.setCustomMenuHTML(
      "<form action='/radar'  method='get'><button>Configure Radar</button></form><br/>\n"
      "<form action='/locs'   method='get'><button>Locations</button></form><br/>\n"
      "<form action='/update' method='get'><button>OTA Update</button></form><br/>\n");
  s_wm.setWebServerCallback(setupRadarRoutes);
  s_wm_configured = true;
}

void startLanWebPortal() {
  if (!wifiLinkUp() || s_wm.getWebPortalActive() ||
      s_wm.getConfigPortalActive()) {
    return;
  }
  WiFi.mode(WIFI_STA);
  s_wm.setConfigPortalBlocking(false);
#ifdef WM_MDNS
  MDNS.end();
  if (MDNS.begin(config::kPortalHostname)) {
    MDNS.addService("http", "tcp", 80);
  }
#endif
  s_wm.startWebPortal();
  Serial.printf("LAN config: http://%s.local or http://%s\n",
                config::kPortalHostname, WiFi.localIP().toString().c_str());
}

void stopLanWebPortal() {
  if (!s_wm.getWebPortalActive()) {
    return;
  }
  s_wm.stopWebPortal();
#ifdef WM_MDNS
  MDNS.end();
#endif
}

void prepareSta() {
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setAutoReconnect(true);
}

void startStaConnect(const String& ssid, const String& pass) {
  prepareSta();
  if (ssid.length() > 0) {
    WiFi.begin(ssid.c_str(), pass.c_str());
  } else {
    WiFi.begin();
  }
}

bool waitForLinkWithUi(const char* ssid_for_ui, unsigned long attempt_ms) {
  const unsigned long deadline = millis() + attempt_ms;
  while (millis() < deadline) {
    if (wifiLinkUp()) {
      return true;
    }
    bootButtonPollLongPress();
    statusScreenConnectingTick();
    delay(config::kWifiConnectingFrameMs);
  }
  return wifiLinkUp();
}

bool tryConnectWithUi(const String& ssid, const String& pass, bool show_ui) {
  if (wifiLinkUp()) {
    return true;
  }

  const char* ui_ssid = ssid.length() > 0 ? ssid.c_str() : "network";
  if (show_ui) {
    statusScreenConnectingBegin(ui_ssid);
  }

  for (uint8_t attempt = 1; attempt <= config::kWifiConnectAttempts; ++attempt) {
    if (attempt > 1) {
      Serial.printf("WiFi connect retry %u/%u\n", attempt,
                    config::kWifiConnectAttempts);
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(400);
    }

    startStaConnect(ssid, pass);

    if (waitForLinkWithUi(ui_ssid, config::kWifiConnectAttemptMs)) {
      return true;
    }
  }

  return false;
}

bool connectSavedNetwork(bool show_ui) {
  if (!storedWifiCredentials()) {
    return false;
  }

  ensureWifiManager();
  const String ssid = s_wm.getWiFiSSID();
  if (ssid.length() == 0) {
    return false;
  }
  const String pass = s_wm.getWiFiPass();
  return tryConnectWithUi(ssid, pass, show_ui);
}

bool openConfigPortal() {
  stopLanWebPortal();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(50);
  statusScreenPortal();
  s_wm.setConfigPortalBlocking(false);
  s_wm.startConfigPortal(config::kPortalApName);
  while (s_wm.getConfigPortalActive()) {
    bootButtonPollLongPress();
    if (s_wm.process()) {
      return true;
    }
    delay(10);
  }
  return wifiLinkUp();
}

}  // namespace

bool wifiShowsSetupScreenOnBoot() {
  if (s_force_config_portal) {
    return true;
  }
  Preferences prefs;
  if (!prefs.begin(kWifiPrefsNamespace, true)) {
    return false;
  }
  const bool pending = prefs.getBool(kPrefsForcePortalKey, false);
  prefs.end();
  return pending;
}

bool wifiBootButtonPressed() {
  return digitalRead(config::kBootPin) == LOW;
}

void bootButtonInit() { initBootButton(); }

bool bootButtonConsumeTap() {
  portENTER_CRITICAL(&s_boot_mux);
  const bool tap = s_boot_tap_pending;
  if (tap) {
    s_boot_tap_pending = false;
  }
  portEXIT_CRITICAL(&s_boot_mux);
  return tap;
}

void bootButtonPollLongPress() {
  if (wifiBootButtonPressed()) {
    portENTER_CRITICAL(&s_boot_mux);
    if (!s_boot_is_down) {
      s_boot_is_down = true;
      s_boot_down_ms = millis();
    }
    const unsigned long down_ms = s_boot_down_ms;
    portEXIT_CRITICAL(&s_boot_mux);

    if (!s_long_press_handled &&
        millis() - down_ms >= config::kBootResetHoldMs) {
      s_long_press_handled = true;
      Serial.println("BOOT held — resetting WiFi");
      wifiResetCredentialsAndReboot();
    }
  } else {
    portENTER_CRITICAL(&s_boot_mux);
    s_boot_is_down = false;
    portEXIT_CRITICAL(&s_boot_mux);
    s_long_press_handled = false;
  }
}

void wifiResetCredentialsAndReboot() {
  resetWifiCredentials();
  statusScreenWifiReset();
  delay(800);
  esp_restart();
}

bool wifiReconnect() {
  initBootButton();
  Serial.println("WiFi reconnecting...");
  return connectSavedNetwork(true);
}

void wifiLoop() {
  ensureWifiManager();
  if (wifiLinkUp()) {
    if (!s_wm.getWebPortalActive() && !s_wm.getConfigPortalActive()) {
      startLanWebPortal();
    }
    if (s_wm.getWebPortalActive() || s_wm.getConfigPortalActive()) {
      bootButtonPollLongPress();
      s_wm.process();
    }
  } else {
    stopLanWebPortal();
  }
}

bool wifiSetupConnect() {
  initBootButton();
  ensureWifiManager();

  const bool force_portal = consumeForceConfigPortal();
  WiFi.setAutoReconnect(false);

  if (force_portal) {
    eraseWifiCredentials();
    WiFi.mode(WIFI_OFF);
    delay(100);
  }

  if (force_portal) {
    Serial.println("Opening WiFi setup portal (after reset)");
    if (openConfigPortal() && wifiLinkUp()) {
      WiFi.setAutoReconnect(true);
      Serial.printf("Connected: %s  IP %s\n", WiFi.SSID().c_str(),
                    WiFi.localIP().toString().c_str());
      return true;
    }
    Serial.println("WiFi connection failed");
    statusScreenConnectFailed();
    return false;
  }

  Serial.println("Connecting to WiFi (portal opens if needed)...");

  if (wifiLinkUp()) {
    WiFi.setAutoReconnect(true);
    Serial.printf("Connected: %s  IP %s\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    return true;
  }

  if (storedWifiCredentials() && connectSavedNetwork(true)) {
    WiFi.setAutoReconnect(true);
    Serial.printf("Connected: %s  IP %s\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    return true;
  }

  if (strlen(config::kWifiFallbackSSID) > 0 &&
      tryConnectWithUi(config::kWifiFallbackSSID, config::kWifiFallbackPass, true)) {
    WiFi.setAutoReconnect(true);
    Serial.printf("Connected: %s  IP %s\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    return true;
  }

  if (storedWifiCredentials()) {
    Serial.println("Saved WiFi could not connect — opening setup portal");
  } else {
    Serial.println("No saved WiFi — opening setup portal");
  }

  if (openConfigPortal() && wifiLinkUp()) {
    WiFi.setAutoReconnect(true);
    Serial.printf("Connected: %s  IP %s\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    return true;
  }

  Serial.println("WiFi connection failed");
  statusScreenConnectFailed();
  return false;
}
