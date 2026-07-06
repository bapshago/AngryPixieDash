# Dash Tablet Setup (Kindle Fire)

How to set up a Fire tablet as the in-car display so it wakes when the
ignition provides USB power and sleeps when power is cut.

## How the pieces fit

- The tablet's USB power comes from an **ignition-switched 12V→USB adapter**
  (use a quality adapter rated 2A+ — the tablet charges while driving the
  screen at full brightness).
- A Fire tablet that is fully **powered off** boots automatically when USB
  power is applied (~1 minute). A **sleeping** tablet wakes instantly, so
  the goal is sleep/wake on ignition, not off/on.
- The ESP32's captive-portal DNS (already in the dashboard firmware) makes
  the tablet treat the car WiFi as "online", so it auto-reconnects to
  `ESP32-EV-Dashboard` without complaints.

## Recommended: Fully Kiosk Browser

Fully Kiosk turns the tablet into a dedicated dashboard display: it boots
straight into the dash page full-screen, wakes the screen when ignition
power appears, and blanks it when power drops.

### Install

1. On the tablet: **Settings → Security & Privacy → Apps from Unknown
   Sources** — allow for the browser you'll download with (Silk).
2. Download the Fully Kiosk Browser APK from <https://www.fully-kiosk.de>
   and install it.

### Configure

Open Fully Kiosk's settings (swipe from left edge → Settings):

| Setting | Where | Value |
|---------|-------|-------|
| Start URL | Web Content Settings | `http://192.168.4.1/` |
| Launch on Boot | Device Management | ON |
| Keep Screen On | Power Settings | ON |
| Screen On when Power Connected | Power Settings | ON (PLUS) |
| Screen Off when Power Disconnected | Power Settings | ON (PLUS) |
| Kiosk Mode (optional) | Kiosk Mode | ON — locks tablet to the dash |

- Grant **device admin** permission when prompted — required for
  "Screen Off when Power Disconnected".
- The two power-trigger settings need the one-time **PLUS license**
  (~$10 per device) — this is the feature that makes the whole
  ignition-follow behavior work.
- Also connect the tablet to the `ESP32-EV-Dashboard` WiFi and tell
  Fire OS to auto-join it.

### Result

Ignition on → screen lights up already showing the dashboard, no lock
screen, no swiping. Ignition off → screen goes dark immediately, tablet
sleeps on battery.

## Fallback: no apps (built-in settings only)

Works, but wakes to the lock screen and needs a manual swipe + browser.

1. **Settings → Device Options → About Fire Tablet** → tap **Serial
   Number** 7 times to unlock Developer Options.
2. **Developer Options → Stay awake**: ON (screen never sleeps while
   charging).
3. **Display → Screen Timeout**: 30 seconds (shortest).
4. **Security**: remove the lock screen PIN.
5. Open Silk to `http://192.168.4.1/` after each wake.

Note: Fire lock screen ads ("Special Offers") get in the way on ad-supported
tablets; Amazon removes them for a one-time ~$15 fee, or Fully Kiosk
bypasses the issue entirely.

## Tips

- Set screen brightness manually (auto-brightness hunts at night).
- Fire OS battery: if the tablet will live in the car through hot summers,
  consider mounting it out of direct sun — heat is what kills tablet
  batteries, not charge cycles.
- The dashboard page itself needs no login and reloads its data every
  500 ms, so a stale page recovers by itself as soon as WiFi reconnects.
