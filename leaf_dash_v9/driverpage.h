const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>Angry Pixie Dash</title>
<style>
    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: 'Segoe UI', Verdana, sans-serif;
      background: #000;
      color: #fff;
    }

    .page {
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 8px 14px 14px;
      gap: 0px;
    }

    .service-link {
      width: 100%;
      display: flex;
      justify-content: center;
      padding-top: 4px;
    }
    .service-link a {
      color: #4488ff;
      text-decoration: none;
      font-size: 0.9em;
    }

    .speed-row {
      display: flex;
      justify-content: center;
      width: 100%;
    }

    .controls-row {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 40px;
      width: 100%;
      margin-top: 0;
    }
    .opmode-icon { font-size: 2.4em; }
    .drive-direction {
      font-size: 2em;
      font-weight: bold;
      color: #555;
      display: flex;
      align-items: center;
      gap: 18px;
    }
    .drive-direction .active {
      color: #4488ff;
      font-size: 2.6em;
    }

    .charge-container {
      width: 100%;
      display: flex;
      justify-content: center;
    }
    .charge-box {
      width: 88%;
      max-width: 580px;
      text-align: center;
    }
    .charge-label {
      font-weight: bold;
      font-size: 1em;
      color: #aaa;
    }
    .charge-bar {
      width: 100%;
      height: 30px;
      background: #2a2a2a;
      border-radius: 15px;
      overflow: hidden;
      margin-top: 5px;
    }
    .charge-fill {
      height: 100%;
      width: 0%;
      background: #00e040;
      border-radius: 15px;
      transition: width 0.3s ease-out, background-color 0.3s ease-out;
    }
    .charge-value {
      font-size: 1em;
      color: #ccc;
    }
    .range-value {
      font-size: 1em;
      color: #4af;
    }

    /* small gauges sit in a single horizontal row */
    .small-gauges-row {
      display: flex;
      flex-direction: row;
      justify-content: center;
      gap: 6px;
      width: 100%;
    }
    #leftColumn, #midColumn, #rightColumn {
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    /* single 12V gauge sits vertically centered between the pairs */
    #midColumn { justify-content: center; }

    /* red anodized bezel band around the whole gauge cluster */
    .gauge-cluster {
      display: flex;
      flex-direction: column;
      align-items: center;
      width: fit-content;
      margin: 0 auto;
      padding: 2px 10px 6px;
      border: 4px solid #c8102e;
      border-radius: 20px;
      box-shadow: 0 0 8px rgba(200,16,46,0.6), inset 0 0 8px rgba(200,16,46,0.35);
    }

    /* gauge boxes – no external label or value divs any more */
    .gauge-box, .gauge-boxsmall {
      background: #000;
      border-radius: 8px;
      text-align: center;
      width: fit-content;
      padding: 0;
    }

    /* needle line – position & size set inline by createGauge */
    .gauge-needle-line {
      position: absolute;
      background: #fff;
      transform-origin: bottom center;
      transition: transform 0.3s ease-out;
    }

    .status-box {
      width: 100%;
      background: #111;
      border-radius: 10px;
      padding: 12px 16px;
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .status-box .row {
      display: flex;
      justify-content: space-between;
      font-size: 1.05em;
    }
    .status-box .row .lbl { color: #aaa; }
    .status-box .row .val { color: #fff; font-weight: bold; }
  </style>
</head>
<body>
<div class="page">

  <!-- Shop wordmark -->
  <div style="text-align:center;padding:8px 0 2px;">
    <span style="font-family:'Futura','Century Gothic','Trebuchet MS',sans-serif;font-size:1.4em;font-weight:700;letter-spacing:0.22em;text-transform:uppercase;background:linear-gradient(90deg,#e05a00,#ff9a3c,#e05a00);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;">Angry Pixie Garage</span>
  </div>

  <!-- Red band matches the anodized bezels in the car -->
  <div class="gauge-cluster">
    <div class="speed-row">
      <div id="speedGauge"></div>
    </div>

    <!-- Voltage+Current | 12V | Inv Temp+Motor Temp -->
    <div class="small-gauges-row">
      <div id="leftColumn"></div>
      <div id="midColumn"></div>
      <div id="rightColumn"></div>
    </div>
  </div>

  <div class="controls-row">
    <div id="opmodeIcon" class="opmode-icon">❓</div>
    <div id="driveDirection" class="drive-direction">
      <span id="dirR">R</span>
      <span id="dirN">N</span>
      <span id="dirF">F</span>
    </div>
  </div>

  <div class="charge-container">
    <div class="charge-box">
      <div style="display:flex;align-items:center;justify-content:center;gap:6px;">
        <span class="charge-label">Charge</span>
        <svg width="38" height="18" viewBox="0 0 38 18" xmlns="http://www.w3.org/2000/svg" style="display:inline-block;vertical-align:middle;">
          <rect x="1" y="2" width="13" height="14" rx="2" fill="none" stroke="#888" stroke-width="1.5"/>
          <path d="M9,3.5 L6,9 L8.5,9 L5.5,14.5 L11,8.5 L8.5,8.5 Z" fill="#FFD700"/>
          <polygon points="19,5 32,9 19,13" fill="#888"/>
        </svg>
      </div>
      <div class="charge-bar"><div class="charge-fill" id="chargeFill"></div></div>
      <div style="display:flex;justify-content:space-between;margin-top:4px;padding:0 2px;">
        <span class="charge-value" id="chargeText">--%</span>
        <span class="range-value" id="rangeText">-- mi</span>
      </div>
      <div id="chargeEtaRow" style="display:none;text-align:center;margin-top:2px;">
        <span class="range-value" id="chargeEta">--</span>
      </div>
    </div>
  </div>

  <div class="status-box">
    <div class="row"><span class="lbl">Plug</span><span class="val" id="plug">--</span></div>
    <div class="row"><span class="lbl">OBC</span><span class="val" id="obc">--</span></div>
    <div class="row" id="inverterErrorRow" style="display:none;">
      <span class="lbl">Inverter</span><span class="val" id="error">--</span>
    </div>
    <div class="row"><span class="lbl">Mode</span><span class="val" id="opmode">--</span></div>
  </div>

  <div class="service-link"><a href="/diagnostics">Service / Diagnostics</a></div>

</div>
<script>
  // Large gauge – speed only
  const gaugeDefs = [
    { label: "Speed", id: "speed", unit: "kph", max: 160, size: 300, radius: 100 }
  ];

  // Small gauges – Current replaces RPM
  // Current gauge is inverted: +150 A (regen) at bottom-left, -250 A (throttle)
  // at bottom-right, so throttle swings the needle clockwise.
  // Zero sits at 37.5% of the arc; green covers the regen side,
  // red covers 200–250 A of discharge.
  const gaugesmallDefs = [
    { label: "Voltage", id: "voltage",   unit: "V",   max: 450 },
    { label: "Current", id: "current",   unit: "A",   min: 150, max: -250,
      greenZones: [[0, 0.375]], redZones: [[0.875, 1.0]] },
    { label: "12V Batt", id: "aux12v",   unit: "V",   min: 10, max: 16,
      redZones: [[0, 0.25], [0.8333, 1.0]] },   // <11.5 V and >15 V
    { label: "Inverter",   id: "invTemp",   unit: "°C", max: 90,
      redZoneStartPercent: 0.8, redZoneStopPercent: 1.0 },
    { label: "Motor",      id: "motorTemp", unit: "°C", max: 90,
      redZoneStartPercent: 0.8, redZoneStopPercent: 1.0 }
  ];

  const opModeIconMap = {
    "Off": "🛑", "Run": "⚡", "Pre Charge": "🔌⏳",
    "Charging": "🔌", "Unknown": "❓"
  };

  // ── helpers ────────────────────────────────────────────────────────────────
  function updateDriveDirection(val) {
    const map = { "-1": "R", "0": "N", "1": "F" };
    const active = map[val?.toString()] ?? "N";
    ["F", "N", "R"].forEach(l => {
      const el = document.getElementById("dir" + l);
      if (el) el.classList.toggle("active", l === active);
    });
  }

  function polarToCartesian(cx, cy, r, deg) {
    const rad = (deg - 90) * Math.PI / 180;
    return { x: cx + r * Math.cos(rad), y: cy + r * Math.sin(rad) };
  }
  function describeArc(cx, cy, r, start, end) {
    const s = polarToCartesian(cx, cy, r, end);
    const e = polarToCartesian(cx, cy, r, start);
    return `M ${s.x} ${s.y} A ${r} ${r} 0 ${end - start <= 180 ? 0 : 1} 0 ${e.x} ${e.y}`;
  }

  // ── gauge creation ─────────────────────────────────────────────────────────
  // Labels and value readout live INSIDE the SVG — no external divs needed.
  function createGauge(container, g, isSmall) {
    const size   = g.size   ?? (isSmall ? 190 : 300);
    const radius = g.radius ?? (isSmall ?  48 : 100);
    const center = size / 2;
    const nw     = isSmall ? 4 : 6;   // needle width px
    const nh     = radius;             // needle height = radius
    const ntop   = center - radius;    // needle top offset (extends upward from center)
    const nleft  = center - nw / 2;

    const arcLen   = radius * Math.PI * 270 / 180;
    g.arcLength    = arcLen;
    const maskPath = describeArc(center, center, radius, 135, 225);

    // Clip the bottom dead-zone arc gap to reduce wasted vertical space
    const clipH = Math.round(center + radius * 0.74 + (isSmall ? 4 : 28));

    const box = document.createElement("div");
    box.className = isSmall ? "gauge-boxsmall" : "gauge-box";
    box.innerHTML = `
      <div style="position:relative;width:${size}px;height:${clipH}px;overflow:hidden;">
        <svg width="${size}" height="${size}" id="${g.id}Svg">
          <g id="${g.id}Ticks"></g>
          <circle id="${g.id}Arc"
            cx="${center}" cy="${center}" r="${radius}"
            stroke="#0096FF" stroke-width="3" fill="none"
            stroke-dasharray="${arcLen}" stroke-dashoffset="${arcLen}"
            transform="rotate(-225 ${center} ${center})"/>
          <path d="${maskPath}" stroke="black" stroke-width="5" fill="none"/>
        </svg>
        <div id="${g.id}Needle" style="position:absolute;top:0;left:0;pointer-events:none;">
          <div class="gauge-needle-line"
            style="width:${nw}px;height:${nh}px;top:${ntop}px;left:${nleft}px;"></div>
        </div>
      </div>`;

    container.appendChild(box);

    const svg = document.getElementById(g.id + "Svg");

    // Colored zone arcs – supports legacy single red zone plus
    // redZones / greenZones arrays of [startPct, stopPct]
    const zones = [];
    if (g.redZoneStartPercent !== undefined)
      zones.push([g.redZoneStartPercent, g.redZoneStopPercent, "#FF0000"]);
    (g.redZones   ?? []).forEach(z => zones.push([z[0], z[1], "#FF0000"]));
    (g.greenZones ?? []).forEach(z => zones.push([z[0], z[1], "#00c840"]));
    zones.forEach(([zs, ze, zc]) => {
      const zp = describeArc(center, center, radius, 225 + zs * 270, 225 + ze * 270);
      const za = document.createElementNS("http://www.w3.org/2000/svg", "path");
      za.setAttribute("d", zp); za.setAttribute("stroke", zc);
      za.setAttribute("stroke-width", "2"); za.setAttribute("fill", "none");
      svg.appendChild(za);
    });

    // Tick marks
    const tg = document.getElementById(g.id + "Ticks");
    for (let deg = 225; deg <= 495; deg += 5) {
      const rad    = (deg - 90) * Math.PI / 180;
      const ox     = center + radius * Math.cos(rad);
      const oy     = center + radius * Math.sin(rad);
      const ir     = deg % 15 === 0 ? radius - 10 : radius - 5;
      const ix     = center + ir * Math.cos(rad);
      const iy     = center + ir * Math.sin(rad);
      const line   = document.createElementNS("http://www.w3.org/2000/svg", "line");
      line.setAttribute("x1", ix); line.setAttribute("y1", iy);
      line.setAttribute("x2", ox); line.setAttribute("y2", oy);
      line.setAttribute("stroke", "#ccc");
      line.setAttribute("stroke-width", deg % 15 === 0 ? "2" : "1");
      tg.appendChild(line);

      // Number labels – large gauges only, every 30°
      if (!isSmall && deg % 30 === 0) {
        const pct = (deg - 225) / 270;
        const val = g.min !== undefined
          ? Math.round(g.min + pct * (g.max - g.min))
          : Math.round(pct * g.max);
        const tr = radius + 18;
        const tx = center + tr * Math.cos(rad);
        const ty = center + tr * Math.sin(rad);
        const t  = document.createElementNS("http://www.w3.org/2000/svg", "text");
        t.setAttribute("x", tx); t.setAttribute("y", ty);
        t.setAttribute("fill", "#fff"); t.setAttribute("font-size", "13");
        t.setAttribute("font-weight", "bold");
        t.setAttribute("text-anchor", "middle"); t.setAttribute("alignment-baseline", "middle");
        tg.appendChild(t);
        t.textContent = val;
      }
    }

    // Gauge name – positioned below center in the dead zone, clear of tick marks
    const nameEl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    nameEl.setAttribute("x", center);
    nameEl.setAttribute("y", center + (isSmall ? 11 : 16));
    nameEl.setAttribute("fill", "#888");
    nameEl.setAttribute("font-size", isSmall ? "10" : "11");
    nameEl.setAttribute("font-family", "Segoe UI, Verdana, sans-serif");
    nameEl.setAttribute("text-anchor", "middle");
    nameEl.textContent = g.label;
    svg.appendChild(nameEl);

    // Unit sub-label – large gauges only, shown on line below the name
    if (!isSmall && g.unit) {
      const unitEl = document.createElementNS("http://www.w3.org/2000/svg", "text");
      unitEl.setAttribute("id", g.id + "UnitLabel");
      unitEl.setAttribute("x", center);
      unitEl.setAttribute("y", center + 62);
      unitEl.setAttribute("fill", "#666");
      unitEl.setAttribute("font-size", "10");
      unitEl.setAttribute("font-family", "Segoe UI, Verdana, sans-serif");
      unitEl.setAttribute("text-anchor", "middle");
      unitEl.textContent = g.unit;
      svg.appendChild(unitEl);
    }

    // Value readout – just below needle pivot (center)
    const valEl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    valEl.setAttribute("id", g.id + "Text");
    valEl.setAttribute("x", center);
    valEl.setAttribute("y", center + (isSmall ? 22 : 40));
    valEl.setAttribute("fill", "#fff");
    valEl.setAttribute("font-size", isSmall ? "14" : "30");
    valEl.setAttribute("font-family", "Segoe UI, Verdana, sans-serif");
    valEl.setAttribute("font-weight", "bold");
    valEl.setAttribute("text-anchor", "middle");
    valEl.setAttribute("dominant-baseline", "middle");
    valEl.textContent = `-- ${g.unit}`;
    svg.appendChild(valEl);
  }

  // ── gauge update ───────────────────────────────────────────────────────────
  function rotateNeedle(id, value, min, max) {
    const pct    = Math.min(Math.max((value - (min ?? 0)) / (max - (min ?? 0)), 0), 1);
    const angle  = pct * 270 - 135;
    const needle = document.querySelector(`#${id}Needle .gauge-needle-line`);
    if (needle) needle.style.transform = `rotate(${angle}deg)`;
  }

  function updateGauge(id, value, max, unit) {
    const arc    = document.getElementById(id + "Arc");
    const txt    = document.getElementById(id + "Text");
    const def    = gaugeDefs.find(g => g.id === id) || gaugesmallDefs.find(g => g.id === id);
    const min    = def?.min ?? 0;
    const arcLen = def.arcLength;
    // Support inverted scales (min > max, e.g. the current gauge)
    const lo     = Math.min(min, max), hi = Math.max(min, max);
    const clamp  = Math.min(Math.max(value, lo), hi);
    const pct    = Math.min(Math.max((clamp - min) / (max - min), 0), 1);
    const offset = arcLen * (1 - pct);
    const color  = pct < 0.05 ? "#FF0000" : "#0096FF";
    if (arc) { arc.setAttribute("stroke-dashoffset", offset); arc.setAttribute("stroke", color); }
    if (txt) txt.textContent = unit ? `${value ?? "--"} ${unit}` : `${value ?? "--"}`;
    rotateNeedle(id, clamp, min, max);
  }

  // ── unit tracking ──────────────────────────────────────────────────────────
  let currentSpeedUnit = null;
  let currentTempUnit  = null;

  async function fetchData() {
    try {
      const response = await fetch('/api/data');
      const data     = await response.json();

      // Speed
      const useMph    = data.use_mph;
      const speedUnit = useMph ? "mph" : "kph";
      const speedMax  = useMph ? 120 : 160;
      const speedVal  = useMph ? (data.vehicle_mph ?? 0) : (data.vehicle_kph ?? 0);
      if (speedUnit !== currentSpeedUnit) {
        currentSpeedUnit = speedUnit;
        const c = document.getElementById("speedGauge");
        c.innerHTML = "";
        gaugeDefs[0].unit = speedUnit;
        gaugeDefs[0].max  = speedMax;
        createGauge(c, gaugeDefs[0], false);
      } else {
        // Keep unit sub-label in sync without recreating the whole gauge
        const ul = document.getElementById("speedUnitLabel");
        if (ul) ul.textContent = speedUnit;
      }
      updateGauge("speed", speedVal, speedMax, "");

      // Temperature
      const useF       = data.use_fahrenheit;
      const tempUnit   = useF ? "°F" : "°C";
      const tempMax    = useF ? 200 : 90;
      const toF        = c => +(c * 9 / 5 + 32).toFixed(1);
      const invTempVal   = useF ? toF(data.inv_temp_c   ?? 0) : (data.inv_temp_c   ?? 0);
      const motorTempVal = useF ? toF(data.motor_temp_c ?? 0) : (data.motor_temp_c ?? 0);
      if (tempUnit !== currentTempUnit) {
        currentTempUnit = tempUnit;
        ["invTemp", "motorTemp"].forEach(id => {
          const def = gaugesmallDefs.find(g => g.id === id);
          def.unit = tempUnit;
          def.max  = tempMax;
          const w  = document.getElementById(id + "Gauge");
          w.innerHTML = "";
          createGauge(w, def, true);
        });
      }

      // All small gauge values
      updateGauge("voltage",   data.voltage_v  ?? 0, 450, "V");
      updateGauge("current",   data.current_a  ?? 0, -250, "A");
      updateGauge("aux12v",    data.aux_12v    ?? 0, 16, "V");
      updateGauge("invTemp",   invTempVal,   tempMax, tempUnit);
      updateGauge("motorTemp", motorTempVal, tempMax, tempUnit);

      // Charge bar + range estimate (22 kWh pack, 3.9 mi/kWh)
      const soc  = data.charge_state ?? 0;
      const fill = document.getElementById("chargeFill");
      fill.style.width           = `${soc}%`;
      fill.style.backgroundColor = soc < 15 ? "#FF0000" : "#00e040";
      document.getElementById("chargeText").textContent = `${soc.toFixed(1)}%`;
      const kwhRemaining = (soc / 100) * 22;
      const rangeVal = useMph
        ? (kwhRemaining * 3.9).toFixed(1) + " mi"
        : (kwhRemaining * 3.9 * 1.60934).toFixed(1) + " km";
      document.getElementById("rangeText").textContent = "~" + rangeVal;

      // Estimated time to full charge – shown only while charging.
      // Charge power (kW) = pack voltage x charge current / 1000;
      // hours = kWh still needed / charge power.
      const etaRow   = document.getElementById("chargeEtaRow");
      const charging = data.op_mode === "Charging" || data.obcvoltstat === 2;
      const chgPowerKW = Math.abs((data.voltage_v ?? 0) * (data.current_a ?? 0)) / 1000;
      const remainKwh  = Math.max(0, (1 - soc / 100) * 22);
      if (charging && chgPowerKW > 0.2 && remainKwh > 0.05) {
        const hrs = remainKwh / chgPowerKW;
        let h = Math.floor(hrs);
        let m = Math.round((hrs - h) * 60);
        if (m === 60) { h += 1; m = 0; }
        document.getElementById("chargeEta").textContent =
          `⚡ ~${h}h ${String(m).padStart(2, "0")}m to full`;
        etaRow.style.display = "block";
      } else {
        etaRow.style.display = "none";
      }

      // Drive direction & op-mode
      updateDriveDirection(data.drive_direction);
      const opMode = data.op_mode ?? "Unknown";
      document.getElementById("opmode").textContent     = opMode;
      document.getElementById("opmodeIcon").textContent = opModeIconMap[opMode] ?? "❓";

      // Status
      document.getElementById("plug").textContent = data.pp_stat ? "Inserted" : "Not Inserted";
      document.getElementById("obc").textContent  = data.obc_status_text ?? "--";
      const hasError = !!data.error;
      document.getElementById("inverterErrorRow").style.display = hasError ? "flex" : "none";
      if (hasError) document.getElementById("error").textContent = "⚠️ FAULT";

    } catch (e) {
      console.error("Fetch error:", e);
    }
  }

  window.onload = () => {
    const left  = document.getElementById("leftColumn");
    const mid   = document.getElementById("midColumn");
    const right = document.getElementById("rightColumn");

    // Speed – large gauge
    createGauge(document.getElementById("speedGauge"), gaugeDefs[0], false);

    // Small gauges: Voltage + Current → left, 12V → middle,
    // Inv Temp + Motor Temp → right
    gaugesmallDefs.forEach(g => {
      if (g.id === "voltage" || g.id === "current") {
        createGauge(left, g, true);
      } else if (g.id === "aux12v") {
        createGauge(mid, g, true);
      } else {
        const w = document.createElement("div");
        w.id = g.id + "Gauge";
        right.appendChild(w);
        createGauge(w, g, true);
      }
    });

    fetchData();
    setInterval(fetchData, 500);
  };
</script>
</body>
</html>
)rawliteral";
