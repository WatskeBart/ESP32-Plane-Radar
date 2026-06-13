"use strict";

const $ = (id) => document.getElementById(id);
const statusEl = $("status");
const prog = $("prog");

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

// --- OTA upload (XHR so we get progress) ---
$("ota-form").addEventListener("submit", (ev) => {
  ev.preventDefault();
  const file = $("fw").files[0];
  if (!file) {
    setStatus("Choose a .bin file first.", false);
    return;
  }
  const fd = new FormData();
  fd.append("firmware", file, file.name);

  const xhr = new XMLHttpRequest();
  prog.style.display = "block";
  prog.value = 0;
  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) prog.value = Math.round((e.loaded / e.total) * 100);
  };
  xhr.onload = () => {
    if (xhr.status === 200) {
      setStatus("Update OK — device is rebooting into the new firmware.", true);
    } else {
      setStatus("Update failed (HTTP " + xhr.status + ").", false);
    }
  };
  xhr.onerror = () => setStatus("Update failed — connection lost.", false);
  xhr.open("POST", "/update");
  setStatus("Uploading firmware…");
  xhr.send(fd);
});

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

loadInfo();
