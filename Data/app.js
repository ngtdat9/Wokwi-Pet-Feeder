async function fetchStatus() {
  try {
    const res = await fetch("/api/status");
    if (!res.ok) throw new Error("HTTP " + res.status);
    const data = await res.json();

    // Weight
    const w = Number(data.weight || 0);
    document.getElementById("weightValue").textContent = w.toFixed(1);

    // Feeding state + badge
    const feeding = !!data.feedingActive;
    const feedingText = feeding ? "Feeding in progress" : "Idle";
    document.getElementById("feedingState").textContent = feedingText;

    const badgeDot = document.getElementById("badgeDot");
    const badgeText = document.getElementById("badgeText");
    badgeDot.classList.toggle("busy", feeding);
    badgeText.textContent = feeding ? "Servo running" : "Ready to feed";

    // Next time
    document.getElementById("nextTimeLabel").textContent = data.nextTime || "None";

    // Slots
    if (Array.isArray(data.slots)) {
      data.slots.forEach((slot, i) => {
        const t = document.getElementById(`slot${i}-time`);
        const wInput = document.getElementById(`slot${i}-weight`);
        const row = document.getElementById(`slot-row-${i}`);
        if (!t || !wInput || !row) return;

        const hh = String(slot.hour ?? 0).padStart(2, "0");
        const mm = String(slot.minute ?? 0).padStart(2, "0");
        t.value = `${hh}:${mm}`;
        wInput.value = slot.weight ?? 0;

        const active = !!slot.active && Number(slot.weight || 0) > 0;
        row.classList.toggle("inactive", !active);
      });
    }
  } catch (err) {
    console.error("Status error:", err);
  }
}

async function manualFeed() {
  const amountEl = document.getElementById("manualAmount");
  const msgEl = document.getElementById("manualMsg");
  const amount = parseFloat(amountEl.value || "0");

  if (isNaN(amount) || amount <= 0) {
    msgEl.textContent = "Enter a portion > 0 g";
    return;
  }

  try {
    const res = await fetch("/api/manual-feed?amount=" + amount, {
      method: "POST",
    });
    if (!res.ok) {
      msgEl.textContent = "Error: " + (await res.text());
    } else {
      msgEl.textContent = "Feeding started!";
    }
  } catch (err) {
    msgEl.textContent = "Network error";
  }
}

async function saveSlot(index) {
  const tInput = document.getElementById(`slot${index}-time`);
  const wInput = document.getElementById(`slot${index}-weight`);
  if (!tInput || !wInput) return;

  const [hh, mm] = (tInput.value || "00:00").split(":");
  const weight = parseFloat(wInput.value || "0");

  const params = new URLSearchParams({
    index,
    hour: hh || "0",
    minute: mm || "0",
    weight: String(weight),
  });

  try {
    const res = await fetch("/api/set-slot?" + params.toString(), {
      method: "POST",
    });
    if (!res.ok) {
      alert("Error: " + (await res.text()));
    } else {
      alert("Slot " + (index + 1) + " saved!");
      fetchStatus();
    }
  } catch (err) {
    alert("Network error");
  }
}

document.addEventListener("DOMContentLoaded", () => {
  // Manual feed
  document.getElementById("manualBtn").addEventListener("click", manualFeed);

  // Slot saves
  document.querySelectorAll(".slot-save").forEach((btn) => {
    btn.addEventListener("click", () => {
      const idx = parseInt(btn.dataset.index, 10);
      saveSlot(idx);
    });
  });

  // Poll status
  fetchStatus();
  setInterval(fetchStatus, 1000);
});
