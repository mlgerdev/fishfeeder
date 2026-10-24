/**
 * FishFeeder UI node — local secrets
 * Copy this file to secrets.h (it is git-ignored!) and fill in real values.
 *   cp src/ui_node/secrets.example.h src/ui_node/secrets.h
 */
#pragma once

// --- WiFi (2.4 GHz only — ESP32/ESP8266 cannot see 5 GHz) ---
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

// --- Timezone, POSIX TZ format (NOTE: the sign is INVERTED vs UTC offset) ---
//   Asia/Manila (UTC+8, no DST)          -> "PST-8"
//   Central Europe (summer time)         -> "CET-1CEST,M3.5.0,M10.5.0/3"
//   US Eastern (summer time)             -> "EST5EDT,M3.2.0,M11.1.0"
#define TZ_STRING "PST-8"

// --- MAC address of the ESP8266 relay node ---
// Flash the relay node first, open its serial monitor (115200 baud) and copy
// the printed MAC. Example: 5C:CF:7F:12:34:56
#define RELAY_MAC                                                                 \
  { 0x5C, 0xCF, 0x7F, 0x00, 0x00, 0x00 }
