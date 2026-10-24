/**
 * FishFeeder relay node — local secrets
 * Copy this file to secrets.h (it is git-ignored!) and fill in real values.
 *   cp src/relay_node/secrets.example.h src/relay_node/secrets.h
 */
#pragma once

// --- WiFi (2.4 GHz only) — must be the SAME network as the UI node so both
// radios land on the same channel (ESP-NOW requirement). ---
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"
