
const char DIAGNOSTICS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Diagnostics</title>
  <style>
    body {
      font-family: 'Segoe UI', Verdana, sans-serif;
      background: #000000;
      color: #fff;
      padding: 20px;
    }
          
      nav {
        
        display: flex;
        justify-content: flex-end;
        margin-bottom: 20px;
      }
      
      nav a {
        color: #00f;
        margin-left: 15px; /* Use margin-left instead of margin-right for spacing between links */
        text-decoration: none;
        font-weight: bold;
      }

    .data-box {
      background: #fff;
      color: #000;
      padding: 15px;
      border-radius: 8px;
      box-shadow: 0 0 5px rgba(0,0,0,0.1);
      margin-bottom: 20px;
    }
    .label {
      font-weight: bold;
    }
    .unit-toggle {
      display: flex;
      gap: 10px;
      align-items: center;
      margin-top: 5px;
    }
    .unit-toggle button {
      padding: 6px 18px;
      border-radius: 6px;
      border: 2px solid #555;
      background: #222;
      color: #fff;
      font-size: 1em;
      cursor: pointer;
    }
    .unit-toggle button.active {
      background: #0060ff;
      border-color: #0060ff;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <nav>
    <a href="/">Driver View</a>
  </nav>
  <h1>Diagnostics / Service</h1>

  <div class="data-box">
    <div class="unit-toggle">
      <span class="label">Speed Unit:</span>
      <button id="btnKph" onclick="setUnit('kph')">KPH</button>
      <button id="btnMph" onclick="setUnit('mph')">MPH</button>
    </div>
    <div class="unit-toggle" style="margin-top:12px;">
      <span class="label">Temperature:</span>
      <button id="btnC" onclick="setTemp('c')">°C</button>
      <button id="btnF" onclick="setTemp('f')">°F</button>
    </div>
  </div>

  <div class="data-box">
    <div><span class="label">DC Bus Voltage:</span> <span id="voltage">--</span> V</div>
    <div><span class="label">12V Battery:</span> <span id="aux_12v">--</span> V</div>
    <div><span class="label">ZombieVerter 0x31A raw:</span> <span id="raw31a" style="font-family:monospace;">--</span></div>
    <div><span class="label">Inverter Temp:</span> <span id="inv_temp">--</span></div>
    <div><span class="label">Motor Temp:</span> <span id="motor_temp">--</span></div>
    <div><span class="label">State of Charge:</span> <span id="stateofcharge" class="status">--</span></div>
    <div><span class="label">Mode:</span> <span id="opmode" class="status">--</span></div>
    <div><span class="label">Drive Direction:</span> <span id="drive_direction" class="status">--</span></div>
    <div><span class="label">Plug Status:</span> <span id="plug" class="status">--</span></div>
    <div><span class="label">OBC Status:</span> <span id="obc" class="status">--</span></div>
    <div><span class="label">Inverter Error:</span> <span id="error" class="status">--</span></div>
    <div><span class="label">State of Charge:</span> <span id="stateofcharge" class="status">--</span></div>
    <div id="error_box" style="display:none;"><span class="label">Inverter Error:</span> <span id="error">FAULT</span></div>
  </div>
  <script>
    async function setUnit(unit) {
      await fetch('/api/setunit?unit=' + unit, { method: 'POST' });
      updateUnitButtons(unit === 'mph');
    }

    async function setTemp(unit) {
      await fetch('/api/settemp?unit=' + unit, { method: 'POST' });
      updateTempButtons(unit === 'f');
    }

    function updateUnitButtons(useMph) {
      document.getElementById("btnKph").classList.toggle("active", !useMph);
      document.getElementById("btnMph").classList.toggle("active", useMph);
    }

    function updateTempButtons(useF) {
      document.getElementById("btnC").classList.toggle("active", !useF);
      document.getElementById("btnF").classList.toggle("active", useF);
    }

    function toF(c) { return c == null ? null : +(c * 9 / 5 + 32).toFixed(1); }

    async function fetchData() {
      try {
        const response = await fetch('/api/data');
        const data = await response.json();
        const useF = data.use_fahrenheit;
        const tempUnit = useF ? '°F' : '°C';
        const invTemp   = useF ? toF(data.inv_temp_c)   : data.inv_temp_c;
        const motorTemp = useF ? toF(data.motor_temp_c) : data.motor_temp_c;

        document.getElementById("voltage").textContent = data.voltage_v ?? '--';
        document.getElementById("aux_12v").textContent = data.aux_12v ?? '--';
        document.getElementById("raw31a").textContent = data.raw_31a ?? 'no 0x31A frames received';
        document.getElementById("inv_temp").textContent = (invTemp ?? '--') + ' ' + tempUnit;
        document.getElementById("motor_temp").textContent = (motorTemp ?? '--') + ' ' + tempUnit;
        document.getElementById("stateofcharge").textContent = data.charge_state ?? '--';
        document.getElementById("opmode").textContent = data.op_mode ?? '--';
        document.getElementById("drive_direction").textContent = data.drive_direction ?? '--';
        document.getElementById("plug").textContent = data.pp_stat ? 'Inserted' : 'Not Inserted';
        document.getElementById("obc").textContent = data.obc_status_text ?? '--';
        document.getElementById("error").textContent = data.error ? 'FAULT' : 'OK';

        if (data.error) {
          document.getElementById("error_box").style.display = "block";
        } else {
          document.getElementById("error_box").style.display = "none";
        }

        updateUnitButtons(data.use_mph);
        updateTempButtons(data.use_fahrenheit);
      } catch (e) {
        console.error('Error fetching diagnostics data:', e);
      }
    }

    setInterval(fetchData, 1000);
    fetchData();
  </script>
</body>
</html>
)rawliteral";