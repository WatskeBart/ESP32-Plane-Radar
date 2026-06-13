#pragma once

namespace services::web {

/**
 * Mount the LittleFS partition holding the Plane Radar web assets
 * (radar/locs/device pages + css/js). Call once early in setup().
 * Returns false if the filesystem is missing; pages then 404.
 */
bool begin();

/**
 * The custom web UI runs only in station mode (device on your network),
 * on its own WebServer at port 80 — WiFiManager's portal is not used here.
 */
void startStaServer();
void stopStaServer();
void handleClient();
bool serverActive();

}  // namespace services::web
