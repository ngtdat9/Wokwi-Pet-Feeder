#pragma once
#include <pgmspace.h>

// Minimal compressed HTML that inlines CSS + JS for the fancy UI.
// If you want to tweak the look later, we can split it out again.

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<title>Pet Feeder Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0" />
<style>
:root{--bg:#0f172a;--bg-soft:#020617;--card:#020617;--accent:#22c55e;--accent2:#38bdf8;--text:#e5e7eb;--muted:#9ca3af;--border:rgba(148,163,184,.3);--radius-lg:18px}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:radial-gradient(circle at top,#1e293b 0,#020617 45%,#000 100%);color:var(--text);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px}
.app-shell{width:100%;max-width:980px;background:linear-gradient(135deg,rgba(15,23,42,.98),rgba(15,23,42,.92));border-radius:26px;box-shadow:0 26px 70px rgba(0,0,0,.65),inset 0 0 0 1px rgba(148,163,184,.15);padding:22px 24px 26px;border:1px solid rgba(148,163,184,.4);backdrop-filter:blur(18px)}
.app-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:18px;gap:16px}
.title-block{display:flex;align-items:center;gap:12px}
.logo-pill{width:40px;height:40px;border-radius:999px;background:radial-gradient(circle at 30% 20%,#bbf7d0,#22c55e 40%,#14532d 100%);display:flex;align-items:center;justify-content:center;box-shadow:0 0 0 1px rgba(22,163,74,.6),0 16px 35px rgba(22,163,74,.5)}
.logo-pill span{font-size:22px}
.title-text h1{font-size:22px;letter-spacing:.03em;display:flex;align-items:center;gap:8px}
.title-badge{font-size:10px;text-transform:uppercase;letter-spacing:.12em;padding:3px 8px;border-radius:999px;border:1px solid rgba(148,163,184,.6);color:var(--muted)}
.title-text p{font-size:12px;color:var(--muted);margin-top:2px}
.header-meta{text-align:right;font-size:12px;color:var(--muted)}
.status-dot{width:8px;height:8px;border-radius:999px;margin-right:6px;background:radial-gradient(circle at 30% 20%,#bbf7d0,#22c55e 70%,#166534 100%);box-shadow:0 0 0 1px rgba(22,163,74,.7);display:inline-block}
.ip-line{margin-top:4px}
.grid{display:grid;grid-template-columns:minmax(0,1.25fr) minmax(0,1fr);gap:16px}
@media(max-width:780px){.app-shell{border-radius:20px;padding:16px 14px 20px}.grid{grid-template-columns:minmax(0,1fr)}.header-meta{text-align:left}.app-header{flex-direction:column;align-items:flex-start}}
.card{background:radial-gradient(circle at top left,rgba(15,23,42,.9),#020617);border-radius:var(--radius-lg);border:1px solid var(--border);padding:14px 14px 16px;position:relative;overflow:hidden}
.card::before{content:"";position:absolute;inset:0;opacity:.4;background:radial-gradient(circle at top right,rgba(56,189,248,.18),transparent 55%);pointer-events:none}
.card-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;position:relative;z-index:1}
.card-title{font-size:13px;text-transform:uppercase;letter-spacing:.14em;color:var(--muted)}
.pill{font-size:11px;padding:3px 10px;border-radius:999px;border:1px solid rgba(148,163,184,.5);color:var(--muted)}
.main-stat{display:flex;align-items:baseline;gap:6px;margin-bottom:6px;position:relative;z-index:1}
.main-stat strong{font-size:28px;font-weight:600}
.main-stat span{font-size:13px;color:var(--muted)}
.sub-row{display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-top:6px;position:relative;z-index:1}
.badge-text{font-size:13px;color:#e5e7eb}
.slots-list{list-style:none;display:flex;flex-direction:column;gap:6px;padding-top:4px;position:relative;z-index:1}
.slot-item{display:flex;align-items:center;justify-content:space-between;padding:7px 9px;border-radius:999px;background:radial-gradient(circle at left,rgba(30,64,175,.45),rgba(15,23,42,.9));border:1px solid rgba(59,130,246,.5);font-size:12px}
.slot-item.inactive{background:rgba(15,23,42,.95);border-style:dashed;border-color:rgba(75,85,99,.7);color:var(--muted)}
.slot-left{display:flex;flex-direction:column;gap:2px}
.chip-label{font-size:11px;text-transform:uppercase;letter-spacing:.16em;color:var(--muted)}
.slot-left input[type=time]{background:rgba(15,23,42,.9);border-radius:999px;border:1px solid rgba(148,163,184,.7);color:var(--text);font-size:12px;padding:3px 8px;outline:none;font-variant-numeric:tabular-nums}
.slot-right{display:flex;align-items:center;gap:6px}
.slot-right input[type=number]{width:70px;padding:3px 7px;border-radius:999px;border:1px solid rgba(148,163,184,.7);background:rgba(15,23,42,.9);color:var(--text);font-size:12px;outline:none;font-variant-numeric:tabular-nums}
.unit{font-size:11px;color:var(--muted)}
button{border-radius:999px;border:none;padding:7px 16px;font-size:12px;font-weight:500;cursor:pointer;display:inline-flex;align-items:center;gap:6px;background:linear-gradient(135deg,#22c55e,#16a34a);color:#022c22;box-shadow:0 10px 25px rgba(16,185,129,.45),0 0 0 1px rgba(22,163,74,.8);position:relative;z-index:1;transition:transform .08s ease,box-shadow .1s ease,filter .1s ease}
button:hover{filter:brightness(1.05);box-shadow:0 16px 35px rgba(16,185,129,.65),0 0 0 1px rgba(22,163,74,.9);transform:translateY(-1px)}
button:active{transform:translateY(0);box-shadow:0 6px 16px rgba(16,185,129,.35),0 0 0 1px rgba(22,163,74,.9)}
.btn-secondary{background:rgba(15,23,42,.98);color:var(--muted);box-shadow:0 0 0 1px rgba(148,163,184,.6)}
.btn-secondary:hover{box-shadow:0 8px 18px rgba(15,23,42,.85),0 0 0 1px rgba(226,232,240,.9);filter:none;transform:translateY(-1px)}
.manual-card{background:radial-gradient(circle at bottom left,rgba(34,197,94,.24),#020617);border-color:rgba(34,197,94,.8)}
.manual-card::before{background:radial-gradient(circle at top left,rgba(74,222,128,.3),transparent 60%)}
.manual-main{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:8px;position:relative;z-index:1}
.manual-main-left{display:flex;flex-direction:column;gap:2px}
.manual-title{font-size:16px;font-weight:600}
.manual-sub{font-size:11px;color:#d1fae5}
.manual-input-row{display:flex;align-items:center;gap:8px;margin-top:6px}
.manual-input-row input[type=number]{width:80px;padding:4px 7px;border-radius:999px;border:1px solid rgba(148,163,184,.8);background:rgba(15,23,42,.9);color:var(--text);font-size:12px;font-variant-numeric:tabular-nums;outline:none}
.badge-status{display:inline-flex;align-items:center;gap:6px;padding:3px 8px;border-radius:999px;background:rgba(15,23,42,.95);border:1px solid rgba(148,163,184,.6);font-size:11px;position:relative;z-index:1}
.badge-dot{width:8px;height:8px;border-radius:999px;background:#22c55e;box-shadow:0 0 0 1px rgba(22,163,74,.8)}
.badge-dot.busy{background:#f97316;box-shadow:0 0 0 1px rgba(249,115,22,.9)}
.small-note{font-size:11px;color:var(--muted);margin-top:6px;position:relative;z-index:1}
.log-message{font-size:11px;margin-top:4px;min-height:14px;color:#bbf7d0}

/* History list styles */
.history-list{list-style:none;margin:0;padding:0;font-size:11px;color:var(--muted);max-height:140px;overflow-y:auto}
.history-item{display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid rgba(148,163,184,.2)}
.history-item:last-child{border-bottom:none}
.history-type{font-weight:500;color:#e5e7eb}
.history-meta{font-size:10px;color:var(--muted)}
.history-empty{font-size:11px;color:var(--muted);padding:4px 0}
</style>
</head>
<body>
<div class="app-shell">
<header class="app-header">
  <div class="title-block">
    <div class="logo-pill"><span>🐾</span></div>
    <div class="title-text">
      <h1>Pet Feeder <span class="title-badge">ESP32 • Web UI</span></h1>
      <p>Monitor bowl weight, upcoming meals and trigger manual feeding.</p>
    </div>
  </div>
  <div class="header-meta">
    <div><span class="status-dot"></span><span>Online (simulation)</span></div>
    <div class="ip-line">IP: see Serial Monitor</div>
  </div>
</header>
<main class="grid">
<section>
  <div class="card">
    <div class="card-header">
      <div class="card-title">Live Bowl Weight</div>
      <div style="display:flex;align-items:center;gap:8px;">
        <span class="pill">HX711</span>
        <button class="btn-secondary" id="resetBtn" style="padding:4px 10px;font-size:11px;">
          Reset
        </button>
      </div>
    </div>
    <div class="main-stat">
      <strong id="weightValue">0.0</strong><span>grams</span>
    </div>
    <div class="sub-row">
      <div><span class="chip-label">Feeding</span><br /><span id="feedingState" class="badge-text">Idle</span></div>
      <div><span class="chip-label">Next feed</span><br /><span id="nextTimeLabel">--:--</span></div>
    </div>
  </div>
  <div class="card" style="margin-top:14px;">
    <div class="card-header">
      <div class="card-title">Schedule</div>
      <span class="pill">3 slots</span>
    </div>
    <ul class="slots-list">
      <li class="slot-item" id="slot-row-0">
        <div class="slot-left">
          <div class="chip-label">Slot 1</div>
          <input type="time" id="slot0-time" value="08:00" />
        </div>
        <div class="slot-right">
          <input type="number" id="slot0-weight" value="0" min="0" max="5000" />
          <span class="unit">g</span>
          <button class="btn-secondary slot-save" data-index="0">Save</button>
        </div>
      </li>
      <li class="slot-item" id="slot-row-1">
        <div class="slot-left">
          <div class="chip-label">Slot 2</div>
          <input type="time" id="slot1-time" value="12:00" />
        </div>
        <div class="slot-right">
          <input type="number" id="slot1-weight" value="0" min="0" max="5000" />
          <span class="unit">g</span>
          <button class="btn-secondary slot-save" data-index="1">Save</button>
        </div>
      </li>
      <li class="slot-item inactive" id="slot-row-2">
        <div class="slot-left">
          <div class="chip-label">Slot 3</div>
          <input type="time" id="slot2-time" value="18:00" />
        </div>
        <div class="slot-right">
          <input type="number" id="slot2-weight" value="0" min="0" max="5000" />
          <span class="unit">g</span>
          <button class="btn-secondary slot-save" data-index="2">Save</button>
        </div>
      </li>
    </ul>
    <p class="small-note">Set weight to 0g to disable a slot.</p>
  </div>
</section>
<section>
  <div class="card manual-card">
    <div class="card-header">
      <div class="card-title">Manual Feeding</div>
      <div class="badge-status">
        <span class="badge-dot" id="badgeDot"></span>
        <span id="badgeText">Ready to feed</span>
      </div>
    </div>
    <div class="manual-main">
      <div class="manual-main-left">
        <div class="manual-title">Feed now</div>
        <div class="manual-sub">Dispense a one-off portion directly to the bowl.</div>
        <div class="manual-input-row">
          <input type="number" id="manualAmount" value="100" min="0" max="5000" />
          <span>grams</span>
        </div>
      </div>
      <button type="button" id="manualBtn"><span>Start feeding</span><span>⏵</span></button>
    </div>
    <p id="manualMsg" class="log-message">Uses the same logic as the hardware buttons.</p>
  </div>
  <div class="card" style="margin-top:14px;">
    <div class="card-header">
      <div class="card-title">Feed History</div>
      <span class="pill">Recent events</span>
    </div>
    <ul class="history-list" id="historyList">
      <li class="history-empty">No feed events yet.</li>
    </ul>
  </div>
</section>
</main>
</div>
<script>
// Track which slot the user is currently editing
const editingSlot = [false, false, false];

function renderHistory(items) {
  const list = document.getElementById("historyList");
  if (!list) return;

  list.innerHTML = "";

  if (!items || !items.length) {
    list.innerHTML = '<li class="history-empty">No feed events yet.</li>';
    return;
  }

  items.forEach((ev) => {
    const li = document.createElement("li");
    li.className = "history-item";
    li.innerHTML = `
      <div>
        <div class="history-type">${ev.type}</div>
        <div class="history-meta">${ev.time}</div>
      </div>
      <div class="history-meta">
        target ${ev.target}g • final ${ev.final}g
      </div>
    `;
    list.appendChild(li);
  });
}

async function fetchStatus() {
  try {
    const r = await fetch("/api/status");
    if (!r.ok) throw new Error("HTTP " + r.status);
    const d = await r.json();

    // Weight
    const w = Number(d.weight || 0);
    document.getElementById("weightValue").textContent = w.toFixed(1);

    // Feeding state + badge
    const feeding = !!d.feedingActive;
    document.getElementById("feedingState").textContent =
      feeding ? "Feeding in progress" : "Idle";

    const bd = document.getElementById("badgeDot");
    const bt = document.getElementById("badgeText");
    bd.classList.toggle("busy", feeding);
    bt.textContent = feeding ? "Servo running" : "Ready to feed";

    // Next time
    document.getElementById("nextTimeLabel").textContent =
      d.nextTime || "None";

    // Slots
    if (Array.isArray(d.slots)) {
      d.slots.forEach((s, i) => {
        const t   = document.getElementById(`slot${i}-time`);
        const wIn = document.getElementById(`slot${i}-weight`);
        const row = document.getElementById(`slot-row-${i}`);
        if (!t || !wIn || !row) return;

        // ⛔ Don't override while user is editing this slot
        if (!editingSlot[i]) {
          const hh = String(s.hour   ?? 0).padStart(2, "0");
          const mm = String(s.minute ?? 0).padStart(2, "0");
          t.value = `${hh}:${mm}`;
          wIn.value = s.weight ?? 0;

          const active = !!s.active && Number(s.weight || 0) > 0;
          row.classList.toggle("inactive", !active);
        }
      });
    }

    // History
    if (Array.isArray(d.history)) {
      renderHistory(d.history);
    }
  } catch (e) {
    console.error("Status error:", e);
  }
}

async function resetSystem() {
  try {
    const r = await fetch("/api/reset", { method: "POST" });
    if (!r.ok) {
      console.error("Reset error:", await r.text());
      alert("Reset failed");
    } else {
      alert("System reset");
      fetchStatus();
    }
  } catch (e) {
    console.error("Reset network error:", e);
    alert("Reset failed (network error)");
  }
}

async function manualFeed() {
  const aEl = document.getElementById("manualAmount");
  const mEl = document.getElementById("manualMsg");
  const a = parseFloat(aEl.value || "0");

  if (isNaN(a) || a <= 0) {
    mEl.textContent = "Enter a portion > 0 g";
    return;
  }

  try {
    const r = await fetch("/api/manual-feed?amount=" + a, { method: "POST" });
    if (!r.ok)
      mEl.textContent = "Error: " + (await r.text());
    else
      mEl.textContent = "Feeding started!";
  } catch (e) {
    mEl.textContent = "Network error";
  }
}

async function saveSlot(i) {
  const t = document.getElementById(`slot${i}-time`);
  const w = document.getElementById(`slot${i}-weight`);
  if (!t || !w) return;

  const [hh, mm] = (t.value || "00:00").split(":");
  const wt = parseFloat(w.value || "0");

  const p = new URLSearchParams({
    index:  i,
    hour:   hh || "0",
    minute: mm || "0",
    weight: String(wt),
  });

  try {
    const r = await fetch("/api/set-slot?" + p.toString(), { method: "POST" });
    if (!r.ok) {
      alert("Error: " + (await r.text()));
    } else {
      alert("Slot " + (i + 1) + " saved!");
      fetchStatus();
    }
  } catch (e) {
    alert("Network error");
  }
}

document.addEventListener("DOMContentLoaded", () => {
  // Manual feed button
  document.getElementById("manualBtn").addEventListener("click", manualFeed);

  const resetBtn = document.getElementById("resetBtn");
  if (resetBtn) {
    resetBtn.addEventListener("click", resetSystem);
  }

  // Slot save buttons
  document.querySelectorAll(".slot-save").forEach((btn) => {
    btn.addEventListener("click", () => {
      const idx = parseInt(btn.dataset.index, 10);
      saveSlot(idx);
    });
  });

  // Mark slot as editing on focus, clear on blur
  for (let i = 0; i < 3; i++) {
    const timeEl   = document.getElementById(`slot${i}-time`);
    const weightEl = document.getElementById(`slot${i}-weight`);

    [timeEl, weightEl].forEach((el) => {
      if (!el) return;
      el.addEventListener("focus", () => (editingSlot[i] = true));
      el.addEventListener("blur",  () => (editingSlot[i] = false));
    });
  }

  // Initial fetch + periodic refresh
  fetchStatus();
  setInterval(fetchStatus, 2000); // was 1000ms
});
</script>

</body>
</html>
)rawliteral";
