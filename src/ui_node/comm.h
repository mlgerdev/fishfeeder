/**
 * FishFeeder UI node — comm layer: WiFi + NTP + ESP-NOW link to the relay node.
 *
 * ESP-NOW notes:
 *  - The relay node joins the SAME access point so both radios sit on the same
 *    channel (a hard ESP-NOW requirement). Without WiFi both fall back to
 *    channel 1 (FF_ESPNOW_CHANNEL) and the link keeps working.
 *  - Send callback runs on the WiFi task; receive callback likewise. State
 *    shared with loop() is kept to aligned 32-bit words and is race-tolerant.
 */
#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include "shared/config.h"

class Comm {
public:
  enum FeedResult { FEED_NONE, FEED_PENDING, FEED_OK, FEED_FAIL };

  void begin(); // WiFi STA + NTP + ESP-NOW init (non-blocking)
  void loop();  // heartbeat + feed retry state machine

  // --- WiFi / time ---
  bool wifiConnected();
  int wifiRssi();
  String ipStr();
  static void timeBegin(); // configTzTime with secrets TZ_STRING
  static bool timeValid();

  // --- relay link ---
  bool relayOnline() const {
    return _last_ack_ms && (millis() - _last_ack_ms < FF_NODE_TIMEOUT_MS);
  }
  const char *relayMacStr() const { return _mac_str; }
  int lastRssi() const { return _last_rssi; }

  // Fire a feed pulse (non-blocking). False if a feed is already in flight.
  bool sendFeed(uint16_t duration_s, uint8_t source);
  FeedResult lastFeedResult() const { return _feed_result; }
  bool feedInFlight() const { return _feed_active; }
  uint16_t lastFeedDurationS() const { return _last_feed_dur_s; }
  uint8_t lastFeedSource() const { return _last_feed_src; }

private:
  static void onSent(const uint8_t *mac, esp_now_send_status_t status);
  static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data,
                     int len);

  volatile uint32_t _last_ack_ms = 0;
  int _last_rssi = 0;
  char _mac_str[18] = "??:??:??:??:??:??";

  volatile bool _feed_active = false;
  volatile FeedResult _feed_result = FEED_NONE;
  uint16_t _last_feed_dur_s = 0;
  uint8_t _last_feed_src = 0;
};

extern Comm comm;
