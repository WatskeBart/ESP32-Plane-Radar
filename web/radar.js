"use strict";

const $ = (id) => document.getElementById(id);
const statusEl = $("status");

function setStatus(msg, ok) {
  statusEl.textContent = msg || "";
  statusEl.className = "status" + (ok === false ? " err" : ok === true ? " ok" : "");
}

async function load() {
  try {
    const r = await fetch("/api/radar");
    if (!r.ok) throw new Error("HTTP " + r.status);
    const d = await r.json();
    $("rlat").value = Number(d.lat).toFixed(6);
    $("rlon").value = Number(d.lon).toFixed(6);
    $("useMiles").checked = d.useMiles;
    $("showRunways").checked = d.showRunways;
    $("showCount").checked = d.showCount;
    $("showFlight").checked = d.showFlight;
    $("showType").checked = d.showType;
    $("showAlt").checked = d.showAlt;
    $("useMeters").checked = d.useMeters;
    $("watch").value = d.watch || "";
    (d.presets || []).forEach((v, i) => {
      const el = $("p" + i);
      if (el) el.value = Number(v).toFixed(1);
    });
  } catch (e) {
    setStatus("Failed to load settings: " + e.message, false);
  }
}

async function save(ev) {
  ev.preventDefault();
  const body = {
    lat: parseFloat($("rlat").value),
    lon: parseFloat($("rlon").value),
    useMiles: $("useMiles").checked,
    showRunways: $("showRunways").checked,
    showCount: $("showCount").checked,
    showFlight: $("showFlight").checked,
    showType: $("showType").checked,
    showAlt: $("showAlt").checked,
    useMeters: $("useMeters").checked,
    watch: $("watch").value,
    presets: [0, 1, 2, 3].map((i) => parseFloat($("p" + i).value)),
  };
  setStatus("Saving…");
  try {
    const r = await fetch("/api/radar", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    });
    if (!r.ok) throw new Error("HTTP " + r.status);
    setStatus("Saved.", true);
    load();
  } catch (e) {
    setStatus("Save failed: " + e.message, false);
  }
}

$("radar-form").addEventListener("submit", save);
load();
