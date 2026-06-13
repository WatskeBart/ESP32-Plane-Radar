"use strict";

const $ = (id) => document.getElementById(id);
const listEl = $("list");
const statusEl = $("status");
let state = { max: 3, locations: [] };

function setStatus(msg, ok) {
  statusEl.textContent = msg || "";
  statusEl.className = "status" + (ok === false ? " err" : ok === true ? " ok" : "");
}

function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, (c) =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
}

async function load() {
  try {
    const r = await fetch("/api/locations");
    if (!r.ok) throw new Error("HTTP " + r.status);
    state = await r.json();
    render();
  } catch (e) {
    setStatus("Failed to load: " + e.message, false);
  }
}

function render() {
  listEl.innerHTML = "";
  const valid = state.locations.filter((l) => l.valid);
  if (!valid.length) {
    listEl.innerHTML = '<p class="muted">No saved locations yet.</p>';
  }
  for (const loc of valid) {
    const div = document.createElement("div");
    div.className = "loc" + (loc.active ? " active" : "");
    div.innerHTML =
      `<div class="loc-h"><b>${escapeHtml(loc.name || "(unnamed)")}</b>` +
      `${loc.active ? " ●" : ""}</div>` +
      `<div class="muted">${Number(loc.lat).toFixed(6)}, ${Number(loc.lon).toFixed(6)}</div>`;

    const actions = document.createElement("div");
    actions.className = "loc-actions";

    const useBtn = document.createElement("button");
    useBtn.textContent = "Use";
    useBtn.disabled = loc.active;
    useBtn.onclick = () => act({ action: "use", slot: loc.slot });

    const delBtn = document.createElement("button");
    delBtn.textContent = "Delete";
    delBtn.className = "danger";
    delBtn.onclick = () => act({ action: "delete", slot: loc.slot });

    actions.append(useBtn, delBtn);
    div.append(actions);
    listEl.append(div);
  }

  const full = valid.length >= state.max;
  $("add-section").style.display = full ? "none" : "";
  if (full) {
    const note = document.createElement("p");
    note.className = "muted";
    note.textContent = `Maximum of ${state.max} locations saved.`;
    listEl.append(note);
  }
}

async function act(payload) {
  setStatus("Working…");
  try {
    const r = await fetch("/api/locations", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    if (!r.ok) throw new Error("HTTP " + r.status);
    setStatus("", true);
    await load();
  } catch (e) {
    setStatus("Failed: " + e.message, false);
  }
}

$("add-form").addEventListener("submit", (ev) => {
  ev.preventDefault();
  const free = state.locations.find((l) => !l.valid);
  if (!free) {
    setStatus("No free slot available.", false);
    return;
  }
  act({
    action: "save",
    slot: free.slot,
    name: $("name").value,
    lat: parseFloat($("lat").value),
    lon: parseFloat($("lon").value),
  }).then(() => {
    $("name").value = "";
    $("lat").value = "";
    $("lon").value = "";
  });
});

load();
