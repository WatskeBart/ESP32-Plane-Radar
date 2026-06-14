"use strict";

const $ = (id) => document.getElementById(id);
const statusEl = $("status");

function setStatus(msg, ok) {
  statusEl.textContent = msg || "";
  statusEl.className = "status" + (ok === false ? " err" : ok === true ? " ok" : "");
}

function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, (c) =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
}

async function loadInfo() {
  try {
    const r = await fetch("/api/device");
    if (!r.ok) throw new Error("HTTP " + r.status);
    const d = await r.json();
    $("info").innerHTML =
      `<dt>Firmware</dt><dd>${escapeHtml(d.version || "—")}</dd>` +
      `<dt>Hostname</dt><dd>${escapeHtml(d.hostname)}</dd>` +
      `<dt>mDNS</dt><dd>${escapeHtml(d.mdns)}</dd>` +
      `<dt>IP address</dt><dd>${escapeHtml(d.ip)}</dd>` +
      `<dt>Network</dt><dd>${escapeHtml(d.ssid || "—")}</dd>`;
  } catch (e) {
    setStatus("Failed to load device info: " + e.message, false);
  }
}

$("restart").addEventListener("click", async () => {
  if (!confirm("Restart the device now?")) return;
  setStatus("Restarting…");
  try {
    await fetch("/api/restart", { method: "POST" });
  } catch (e) {
    /* expected: connection drops as the device reboots */
  }
  setStatus("Device is restarting — reconnect in a few seconds.", true);
});

$("wifi-reset").addEventListener("click", async () => {
  if (!confirm("Erase WiFi credentials and reboot into setup AP mode?")) return;
  setStatus("Resetting WiFi…");
  try {
    await fetch("/api/wifi/reset", { method: "POST" });
  } catch (e) {
    /* expected: connection drops as the device reboots */
  }
  setStatus("WiFi cleared — connect to the PlaneRadar-Setup AP to reconfigure.", true);
});

$("device-reset").addEventListener("click", async () => {
  if (!confirm("Reset the device and clear all settings?")) return;
  setStatus("Resetting device…");
  try {
    await fetch("/api/device/reset", { method: "POST" });
  } catch (e) {
    /* expected: connection drops as the device reboots */
  }
  setStatus("Device reset — reconnect in a few seconds.", true);
});


loadInfo();
