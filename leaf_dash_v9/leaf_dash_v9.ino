
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "driver/twai.h"
#include "driverpage.h"
#include "diagnosticpage.h"

// ======== USER CONFIG ========
// WiFi credentials live in secrets.h (gitignored).
// First-time setup: copy secrets.h.example to secrets.h and edit it.
#include "secrets.h"
#define CAN_TX_PIN     GPIO_NUM_25
#define CAN_RX_PIN     GPIO_NUM_27
#define CAN_BITRATE    500000
#define REFRESH_MS     500
#define GEAR_RATIO             7.94f
#define TIRE_CIRCUMFERENCE_M   1.975f

WebServer server(80);

// Captive-portal DNS: answer every lookup with our own IP so tablets
// believe the AP has internet and stay connected automatically.
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Shared state updated by CAN receiver
volatile int32_t g_voltage_V = 0;
volatile int32_t g_motor_rpm = 0;
volatile bool    g_error = false;
volatile float   g_inv_temp_c = NAN;
volatile float   g_motor_temp_c = NAN;
volatile uint8_t g_OBCVoltStat = 0;
volatile uint8_t g_PlugStat = 0;
volatile bool    g_PPStat = false;
volatile float   g_batt_SOC = 0;
volatile float   g_current = 0;
volatile int32_t g_drivedir = 0;
volatile int32_t g_opmode = 0;
volatile float   g_aux_12v = NAN;   // 12V aux battery voltage
volatile uint8_t g_31a_raw[8] = {0};  // last raw 0x31A frame, for diagnostics
volatile uint8_t g_31a_dlc = 0;

bool TEST_MODE = false; // sends test data for dev
bool g_use_mph = false;
bool g_use_fahrenheit = false;

static inline float fahrenheit_to_celsius(uint8_t f) {
  return (float(f) - 32.0f) * (5.0f / 9.0f);
}

static inline float rpm_to_kph(int32_t rpm) {
  if (GEAR_RATIO <= 0.01f || TIRE_CIRCUMFERENCE_M <= 0.01f) return NAN;
  return (float)rpm * (TIRE_CIRCUMFERENCE_M / 60.0f) * (3.6f / GEAR_RATIO);
}
static inline float kph_to_mph(float kph) { return kph * 0.621371f; }

bool twai_init() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.rx_queue_len = 64;
  g_config.tx_queue_len = 16;

  twai_timing_config_t t_config;
  switch (CAN_BITRATE) {
    case 250000: t_config = TWAI_TIMING_CONFIG_250KBITS(); break;
    case 500000: t_config = TWAI_TIMING_CONFIG_500KBITS(); break;
    case 1000000: t_config = TWAI_TIMING_CONFIG_1MBITS(); break;
    default: t_config = TWAI_TIMING_CONFIG_500KBITS(); break;
  }

  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) return false;
  if (twai_start() != ESP_OK) return false;

  return true;
}

void can_rx_task(void *param) {
  const TickType_t timeout = pdMS_TO_TICKS(50);

  while (true) {
    twai_message_t msg;
    esp_err_t res = twai_receive(&msg, timeout);

    if (res == ESP_OK && !msg.rtr && msg.data_length_code >= 6) {
      const uint32_t id = msg.identifier;
      const uint8_t *bytes = msg.data;
      
if (id == 0x1DA) {
  int32_t raw_voltage = ((int32_t)bytes[0] << 2) | (bytes[1] >> 6);
  float voltage = raw_voltage * 0.5f;

  int16_t parsed_speed = (int16_t)((bytes[4] << 8) | bytes[5]);
  float speed = (parsed_speed == 0x7FFF ? 0.0f : parsed_speed * 0.5f);

  bool error = ((bytes[6] & 0xB0) != 0x00);

  g_voltage_V = voltage;
  g_motor_rpm = speed;
  g_error = error;
}

      else if (id == 0x55A) {
        g_inv_temp_c = fahrenheit_to_celsius(bytes[2]);
        g_motor_temp_c = fahrenheit_to_celsius(bytes[1]);
      }
      else if (id == 0x390) {
        uint8_t OBCVoltStat = (bytes[3] >> 3) & 0x03;
        uint8_t PlugStat = bytes[5] & 0x0F;
        bool PPStat = (PlugStat == 0x08);

        g_OBCVoltStat = OBCVoltStat;
        g_PlugStat = PlugStat;
        g_PPStat = PPStat;
      }
     else if (id == 0x55A) {
        g_inv_temp_c = fahrenheit_to_celsius(bytes[2]);
        g_motor_temp_c = fahrenheit_to_celsius(bytes[1]);
     }
     else if (id == 0x55B) {
        float soc = uint16_t(bytes[0] << 2) + uint16_t(bytes[1] >> 6);
        soc = soc*0.1;
        g_batt_SOC = soc;
     }
     else if (id == 0x1DB) {
        float cur = uint16_t(bytes[0] << 3) + uint16_t(bytes[1] >> 5);
        if(cur>1023)cur -=2047; //check if negative
        g_current = cur;
    }
     else if (id == 0x292) {
        // Stock Leaf CAR-CAN: LeadAcidBatteryVoltage in byte 3, 0.1 V/bit
        // (e.g. 0x7F = 12.7 V). Only present if a stock module broadcasts
        // it on this bus; harmless if the frame never appears.
        g_aux_12v = bytes[3] * 0.1f;
    }
      
else if (id == 0x31A) {
  //Can Bus ID Used From ZombieVerter Byte 0 is drive direction and Byte 1 is Operation Mode
    int8_t raw = static_cast<int8_t>(bytes[0]);
    int8_t raw1 = static_cast<int8_t>(bytes[1]);
    int opmode;
    int drivedir;

    if (raw == 0x01) {
        drivedir = 1;
    } else if (raw == 0x00) {
        drivedir = 0;
    } else if (raw == -1 || raw == 0xFF) { // both represent -1 in signed 8-bit
        drivedir = -1;
    } else {
        // Optional: handle unexpected values
        drivedir = 0; // or some error code
    }

    if (raw1 == 0x00) {
        opmode = 0;
    } else if (raw1 == 0x01) {
        opmode = 1;
    }else if (raw1 == 0x02) {
        opmode = 2;
    }else if (raw1 == 0x03) {
        opmode = 3;
    }else if (raw1 == 0x04) {
        opmode = 4;
    } else {
        // Optional: handle unexpected values
        opmode = 0; // or some error code
    }
    g_drivedir = drivedir;
    g_opmode = opmode;

    // Optional 12V reading from ZombieVerter: map the "uaux" spot value
    // onto this frame in the ZombieVerter web UI (CAN TX map: uaux,
    // ID 0x31A / 794 dec, start bit 16, length 16, gain 10) and it lands
    // in bytes 2-3 little-endian at 0.1 V/bit. Zero = not mapped.
    uint16_t raw12 = (uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8);
    if (raw12 != 0) g_aux_12v = raw12 * 0.1f;

    // Keep the raw frame around so diagnostics can show what
    // ZombieVerter is actually sending
    g_31a_dlc = msg.data_length_code;
    for (int i = 0; i < 8; i++)
      g_31a_raw[i] = (i < msg.data_length_code) ? bytes[i] : 0;
}

      taskYIELD();
  }
}
}
void generateTestData() {
  static uint32_t t = 0;
  t += REFRESH_MS;

  g_voltage_V = 215 + int(215 * sin(t / 1000.0));
  g_motor_rpm = 4500 + int(4500 * sin(t / 800.0));
  g_error = (t / 5000) % 2 == 0;

  g_inv_temp_c = 45 + int(45 * sin(t / 1200.0));
  g_motor_temp_c = 45 + int(25 * sin(t / 1000.0));

  g_OBCVoltStat = (t / 3000) % 4;
  g_PlugStat = ((t / 4000) % 2 == 0) ? 0x08 : 0x00;
  g_PPStat = (g_PlugStat == 0x08);
  g_batt_SOC = 50.0f + 50.0f * sin(t / 800.0);
  g_current = int(80 * sin(t / 800.0));
  g_drivedir = int(2 * sin(t / 800.0));
  g_opmode = 2 + int(2 * sin(t / 800.0));
  g_aux_12v = 13.2f + 2.2f * sin(t / 1100.0);  // sweeps 11.0-15.4 V

}


const char* obcStatusText(uint8_t s) {
  switch (s) {
    case 0: return "Idle/No AC";
    case 1: return "AC Present";
    case 2: return "Charging";
    case 3: return "Active/Other";
    default: return "Unknown";
  }
}

const char* opmodeStatusText(uint8_t s) {
  switch (s) {
    case 0: return "Off";
    case 1: return "Run";
    case 2: return "Pre Charge";
    case 3: return "Pre Charge Failed";
    case 4: return "Charging";
    default: return "Unknown";
  }
}

void handleApiData() {
  if (TEST_MODE) generateTestData();

  int32_t voltage_V = g_voltage_V;
  int32_t motor_rpm = g_motor_rpm;
  bool error = g_error;
  float inv_c = g_inv_temp_c;
  float mot_c = g_motor_temp_c;
  uint8_t obc = g_OBCVoltStat;
  uint8_t plug = g_PlugStat;
  bool pp = g_PPStat;
  float stoc = g_batt_SOC;
  float cur = g_current;
  int32_t drivedir = g_drivedir;
  int32_t opmode = g_opmode;
  float aux12 = g_aux_12v;

  float v_kph = rpm_to_kph(motor_rpm);
  float v_mph = isfinite(v_kph) ? kph_to_mph(v_kph) : NAN;

  String j = "{";
  j += "\"voltage_v\":" + String(voltage_V) + ",";
  j += "\"motor_rpm\":" + String(motor_rpm) + ",";
  j += "\"error\":" + String(error ? "true" : "false") + ",";
  j += "\"inv_temp_c\":" + (isfinite(inv_c) ? String(inv_c, 1) : "null") + ",";
  j += "\"motor_temp_c\":" + (isfinite(mot_c) ? String(mot_c, 1) : "null") + ",";
  j += "\"obcvoltstat\":" + String(obc) + ",";
  j += "\"plugstat\":" + String(plug) + ",";
  j += "\"pp_stat\":" + String(pp ? "true" : "false") + ",";
  j += "\"charge_state\":" + String(stoc) + ",";
  j += "\"current_a\":" + String(cur) + ",";
  j += "\"aux_12v\":" + (isfinite(aux12) ? String(aux12, 1) : "null") + ",";

  // Raw 0x31A frame as hex, for diagnosing the ZombieVerter uaux mapping
  if (g_31a_dlc > 0) {
    char rawbuf[32];
    uint8_t raw[8];
    for (int i = 0; i < 8; i++) raw[i] = g_31a_raw[i];
    snprintf(rawbuf, sizeof(rawbuf), "%02X %02X %02X %02X %02X %02X %02X %02X",
             raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7]);
    j += "\"raw_31a\":\"" + String(rawbuf) + "\",";
  } else {
    j += "\"raw_31a\":null,";
  }
  j += "\"drive_direction\":" + String(drivedir) + ",";
  j += "\"op_mode\":\"" + String(opmodeStatusText(opmode)) + "\"" + ",";
  
  if (isfinite(v_kph)) {
    j += "\"vehicle_kph\":" + String(v_kph, 1) + ",";
    j += "\"vehicle_mph\":" + String(v_mph, 1) + ",";
  } else {
    j += "\"vehicle_kph\":null,";
    j += "\"vehicle_mph\":null,";
  }
  j += "\"obc_status_text\":\"" + String(obcStatusText(obc)) + "\",";
  j += "\"use_mph\":" + String(g_use_mph ? "true" : "false") + ",";
  j += "\"use_fahrenheit\":" + String(g_use_fahrenheit ? "true" : "false");
  j += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}


void handleDiagnostics() {
  server.send(200, "text/html", DIAGNOSTICS_HTML);
}

void handleSetUnit() {
  if (server.hasArg("unit")) {
    String unit = server.arg("unit");
    if (unit == "mph") g_use_mph = true;
    else if (unit == "kph") g_use_mph = false;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", String("{\"use_mph\":") + (g_use_mph ? "true" : "false") + "}");
}

void handleSetTemp() {
  if (server.hasArg("unit")) {
    String unit = server.arg("unit");
    if (unit == "f") g_use_fahrenheit = true;
    else if (unit == "c") g_use_fahrenheit = false;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", String("{\"use_fahrenheit\":") + (g_use_fahrenheit ? "true" : "false") + "}");
}


void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[EV Dashboard] Booting...");

  WiFi.mode(WIFI_AP);
  bool ap_ok = WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (ap_ok) {
    Serial.printf("[WiFi] AP started: %s  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("[WiFi] AP start FAILED");
  }

  // Redirect all DNS requests to the ESP32 (captive portal style)
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  if (!twai_init()) {
    Serial.println("[TWAI] Init FAILED");
    while (true) { delay(1000); }
  } else {
    Serial.println("[TWAI] Init OK (started)");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/diagnostics", HTTP_GET, handleDiagnostics);
  server.on("/api/setunit", HTTP_POST, handleSetUnit);
  server.on("/api/settemp", HTTP_POST, handleSetTemp);

  // Connectivity-check handlers so tablets don't flag "no internet"
  server.on("/generate_204", HTTP_GET, []() {          // Android
    server.send(204, "", "");
  });
  server.on("/gen_204", HTTP_GET, []() {               // Android (alt)
    server.send(204, "", "");
  });
  server.on("/hotspot-detect.html", HTTP_GET, []() {   // Apple iOS/iPadOS
    server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });
  server.on("/connecttest.txt", HTTP_GET, []() {       // Windows
    server.send(200, "text/plain", "Microsoft Connect Test");
  });
  server.on("/ncsi.txt", HTTP_GET, []() {              // Windows (legacy)
    server.send(200, "text/plain", "Microsoft NCSI");
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
  server.begin();
  Serial.println("[HTTP] Server started at http://192.168.4.1/");

  xTaskCreatePinnedToCore(can_rx_task, "can_rx", 4096, nullptr, 8, nullptr, APP_CPU_NUM);

  float mph_per_krpm = kph_to_mph(rpm_to_kph(1000));
  float mph_at_10k   = kph_to_mph(rpm_to_kph(10000));
  Serial.printf("[Speed scale] %.3f mph per 1k RPM | ~%.1f mph @ 10k RPM\n",
                mph_per_krpm, mph_at_10k);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  delay(2);
}