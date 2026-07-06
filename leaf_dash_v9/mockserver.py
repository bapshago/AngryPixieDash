"""
Mock server for Angry Pixie Dash — lets you preview the web pages on your PC
without flashing the ESP32.

Usage:
    python mockserver.py

Then open http://localhost:8080 in your browser.
The /api/data endpoint returns animated test data so gauges move.
"""

import http.server
import json
import math
import re
import time
import urllib.parse
from pathlib import Path

HERE = Path(__file__).parent

# ── extract raw HTML from a .h file ──────────────────────────────────────────

def load_html(filename):
    src = (HERE / filename).read_text(encoding="utf-8")
    m = re.search(r'R"rawliteral\((.*?)\)rawliteral"', src, re.DOTALL)
    if not m:
        raise ValueError(f"Could not find rawliteral block in {filename}")
    return m.group(1).encode("utf-8")

DRIVER_HTML      = load_html("driverpage.h")
DIAGNOSTIC_HTML  = load_html("diagnosticpage.h")

# ── shared state ─────────────────────────────────────────────────────────────

state = {"use_mph": False, "use_fahrenheit": False}

# ── animated test data (mirrors generateTestData() in the .ino) ──────────────

GEAR_RATIO          = 7.94
TIRE_CIRCUMFERENCE  = 1.975   # metres

def rpm_to_kph(rpm):
    return rpm * (TIRE_CIRCUMFERENCE / 60.0) * (3.6 / GEAR_RATIO)

def kph_to_mph(kph):
    return kph * 0.621371

OBC_TEXT  = ["Idle/No AC", "AC Present", "Charging", "Active/Other"]
OPMODE_TEXT = ["Off", "Run", "Pre Charge", "Pre Charge Failed", "Charging"]

def make_api_data():
    t = time.time()

    rpm      = 4500 + 4500 * math.sin(t / 0.8)
    voltage  = 215  + 215  * math.sin(t / 1.0)
    error    = (int(t / 5) % 2 == 0)
    inv_temp = 45   + 45   * math.sin(t / 1.2)
    mot_temp = 45   + 25   * math.sin(t / 1.0)
    obc_idx  = int(t / 3)  % 4
    pp_stat  = (int(t / 4) % 2 == 0)
    soc      = 50.0 + 50.0 * math.sin(t / 0.8)
    current  = 80   * math.sin(t / 0.8)
    drivedir = round(2 * math.sin(t / 0.8))
    opmode_i = (2 + round(2 * math.sin(t / 0.8))) % len(OPMODE_TEXT)

    kph = rpm_to_kph(rpm)
    mph = kph_to_mph(kph)

    drivedir = max(-1, min(1, drivedir))

    return {
        "voltage_v":      round(voltage, 1),
        "motor_rpm":      round(rpm, 1),
        "error":          error,
        "inv_temp_c":     round(inv_temp, 1),
        "motor_temp_c":   round(mot_temp, 1),
        "obcvoltstat":    obc_idx,
        "plugstat":       0x08 if pp_stat else 0x00,
        "pp_stat":        pp_stat,
        "charge_state":   round(soc, 1),
        "current_a":      round(current, 1),
        "aux_12v":        round(13.2 + 2.2 * math.sin(t / 1.1), 1),
        "raw_31a":        "%02X %02X %02X %02X 00 00 00 00" % (
                              drivedir & 0xFF, opmode_i,
                              int((13.2 + 2.2 * math.sin(t / 1.1)) * 10) & 0xFF,
                              (int((13.2 + 2.2 * math.sin(t / 1.1)) * 10) >> 8) & 0xFF),
        "drive_direction": drivedir,
        "op_mode":        OPMODE_TEXT[opmode_i],
        "vehicle_kph":    round(kph, 1),
        "vehicle_mph":    round(mph, 1),
        "obc_status_text": OBC_TEXT[obc_idx],
        "use_mph":         state["use_mph"],
        "use_fahrenheit":  state["use_fahrenheit"],
    }

# ── request handler ───────────────────────────────────────────────────────────

class Handler(http.server.BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):
        print(f"  {self.path}  →  {args[1]}")

    def send_response_body(self, code, content_type, body):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", len(body))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?")[0]
        if path == "/":
            self.send_response_body(200, "text/html; charset=utf-8", DRIVER_HTML)
        elif path == "/diagnostics":
            self.send_response_body(200, "text/html; charset=utf-8", DIAGNOSTIC_HTML)
        elif path == "/api/data":
            self.send_response_body(200, "application/json", json.dumps(make_api_data()))
        else:
            self.send_response_body(404, "text/plain", "Not found")

    def do_POST(self):
        path = self.path.split("?")[0]
        qs = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
        if path == "/api/setunit":
            unit = qs.get("unit", [""])[0]
            if unit == "mph":
                state["use_mph"] = True
            elif unit == "kph":
                state["use_mph"] = False
            self.send_response_body(200, "application/json",
                                    json.dumps({"use_mph": state["use_mph"]}))
        elif path == "/api/settemp":
            unit = qs.get("unit", [""])[0]
            if unit == "f":
                state["use_fahrenheit"] = True
            elif unit == "c":
                state["use_fahrenheit"] = False
            self.send_response_body(200, "application/json",
                                    json.dumps({"use_fahrenheit": state["use_fahrenheit"]}))
        else:
            self.send_response_body(404, "text/plain", "Not found")


if __name__ == "__main__":
    PORT = 8080
    server = http.server.HTTPServer(("localhost", PORT), Handler)
    print(f"Angry Pixie Dash mock server running at http://localhost:{PORT}")
    print("  /              → Driver page")
    print("  /diagnostics   → Diagnostics / Service page")
    print("Press Ctrl+C to stop.\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
