# Angry Pixie Dash

A WiFi EV dashboard for a Porsche 928 electric conversion, built on an ESP32.
The ESP32 reads Nissan Leaf / ZombieVerter CAN bus data and serves a live web
dashboard designed for an iPad (10.9", portrait) mounted in the dash.

## Features

- Large speed gauge with KPH/MPH toggle (0–160 / 0–120)
- Small gauge cluster: pack voltage, battery current, 12V aux battery,
  inverter temp, motor temp (°C/°F toggle, 0–90 / 0–200)
- Inverted current gauge: throttle swings clockwise (to −250 A),
  regen swings left (to +150 A) with green regen zone and 200–250 A redline
- Gear direction indicator (R/N/F) and op-mode status
- State of charge bar with range estimate (22 kWh pack, 3.9 mi/kWh)
- Estimated time-to-full while charging
- Charge port side indicator
- Service/diagnostics page with unit toggles and raw readings
- Red bezel band styling to match the car's anodized gauge bezels
- Captive-portal DNS + OS connectivity-check endpoints so tablets treat
  the AP as "online" and reconnect to it automatically

## Hardware

- Angry Pixie UniBoard (ESP32) — schematic, gerbers, BOM and pin map in [hardware/](hardware/)
- CAN (TWAI) at 500 kbit/s
- ZombieVerter VCU driving Nissan Leaf inverter, PDM (charger/DC-DC) and BMS
- WiFi access point: SSID/password set in `secrets.h`, dashboard at `http://192.168.4.1/`

### CAN messages read

| ID    | Data                                        |
|-------|---------------------------------------------|
| 0x1DA | Pack voltage, motor speed, inverter error   |
| 0x55A | Inverter and motor temperature              |
| 0x390 | OBC / charge plug status                    |
| 0x55B | State of charge                             |
| 0x1DB | Battery current                             |
| 0x31A | ZombieVerter: drive direction, op mode, 12V aux voltage — requires custom TX mappings, see [ZombieVerter_CAN_Mappings.md](ZombieVerter_CAN_Mappings.md) |
| 0x292 | 12V battery voltage (stock Leaf CAR-CAN, if present) |

## Flashing

1. Copy `leaf_dash_v9/secrets.h.example` to `leaf_dash_v9/secrets.h` and set
   your own WiFi SSID and password (the real `secrets.h` is gitignored).
2. Open `leaf_dash_v9/leaf_dash_v9.ino` in the Arduino IDE with an ESP32 board
   package installed and upload. Set `TEST_MODE = true` to generate fake data
   without a CAN bus connected.

## In-car display

See [Tablet_Setup.md](Tablet_Setup.md) for configuring a Fire tablet to
wake with ignition power and sleep when it's cut, and
[ZombieVerter_CAN_Mappings.md](ZombieVerter_CAN_Mappings.md) for the CAN
TX mappings the dash depends on.

## Previewing the web pages without hardware

```
cd leaf_dash_v9
python mockserver.py
```

Then open http://localhost:8080 — the mock server serves the same HTML the
ESP32 does, with animated test data.
