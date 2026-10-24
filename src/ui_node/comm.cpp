#include "comm.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <time.h>
#include "shared/protocol.h"
#include "secrets.h"

Comm comm;

namespace {
uint8_t peer_mac[6] = {RELAY_MAC};
uint32_t seq = 1;

uint32_t last_ping_ms = 0;

// --- feed state machine ---
uint32_t feed_seq = 0;
uint16_t feed_duration_s = 0;
uint8_t feed_source = 0;
int feed_retries = 0;
uint32_t feed_last_try_ms = 0;
const int FEED_MAX_RETRIES = 3;
const uint32_t FEED_RETRY_MS = 1500;

// --- tx/rx handshake flags ---
volatile bool tx_done = false;
volatile esp_now_send_status_t tx_status = ESP_NOW_SEND_SUCCESS;
} // namespace

void Comm::timeBegin() {
  configTzTime(TZ_STRING, "pool.ntp.org", "time.google.com", "time.nist.gov");
}

bool Comm::timeValid() { return time(nullptr) > 1700000000; }

void Comm::begin() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[COMM] WiFi connecting to \"%s\"...\n", WIFI_SSID);

  timeBegin();

  if (esp_now_init() != ESP_OK) {
    Serial.println("[COMM] esp_now_init FAILED");
    return;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, peer_mac, 6);
  peer.channel = 0; // follow the current STA channel
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK)
    Serial.println("[COMM] esp_now_add_peer failed");

  esp_now_register_send_cb(Comm::onSent);
  esp_now_register_recv_cb(Comm::onRecv);

  snprintf(_mac_str, sizeof(_mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4],
           peer_mac[5]);
  Serial.printf("[COMM] relay peer: %s  (self: %s)\n", _mac_str,
                WiFi.macAddress().c_str());
}

bool Comm::wifiConnected() { return WiFi.status() == WL_CONNECTED; }

int Comm::wifiRssi() {
  return wifiConnected() ? WiFi.RSSI() : 0;
}

String Comm::ipStr() {
  return wifiConnected() ? WiFi.localIP().toString() : String("-");
}

// ------------------------------------------------------------------ raw send
static bool raw_send(const ff_packet &p) {
  tx_done = false;
  esp_err_t e = esp_now_send(peer_mac, (const uint8_t *)&p, sizeof(p));
  if (e != ESP_OK)
    return false;
  uint32_t t0 = millis();
  while (!tx_done && millis() - t0 < 100)
    delay(1); // wait briefly for the send callback
  return tx_done && tx_status == ESP_NOW_SEND_SUCCESS;
}

bool Comm::sendFeed(uint16_t duration_s, uint8_t source) {
  if (_feed_active)
    return false;
  _feed_active = true;
  _feed_result = FEED_PENDING;
  _last_feed_dur_s = duration_s;
  _last_feed_src = source;
  feed_seq = seq++;
  feed_duration_s = duration_s;
  feed_source = source;
  feed_retries = 0;
  feed_last_try_ms = 0; // first attempt happens in the next loop()
  return true;
}

void Comm::loop() {
  uint32_t now = millis();

  // heartbeat
  if (now - last_ping_ms >= FF_HEARTBEAT_PERIOD_MS) {
    last_ping_ms = now;
    ff_packet p;
    ff_pack(p, FF_CMD_PING, seq++, 0, 0);
    raw_send(p);
  }

  // feed attempts / timeout
  if (_feed_active && now - feed_last_try_ms >= FEED_RETRY_MS) {
    feed_last_try_ms = now;
    if (feed_retries >= FEED_MAX_RETRIES) {
      _feed_result = FEED_FAIL;
      _feed_active = false;
      Serial.println("[COMM] feed FAILED (no ack)");
    } else {
      feed_retries++;
      ff_packet p;
      ff_pack(p, FF_CMD_FEED, feed_seq, (uint32_t)feed_duration_s * 1000UL,
              feed_source);
      bool ok = raw_send(p);
      Serial.printf("[COMM] feed attempt %d (%ums) -> %s\n", feed_retries,
                    feed_duration_s * 1000, ok ? "tx ok" : "tx fail");
    }
  }
}

// ---------------------------------------------------------------- callbacks
void Comm::onSent(const uint8_t *mac, esp_now_send_status_t status) {
  tx_status = status;
  tx_done = true;
}

void Comm::onRecv(const esp_now_recv_info_t *info, const uint8_t *data,
                  int len) {
  if (len != (int)sizeof(ff_packet))
    return;
  ff_packet p;
  memcpy(&p, data, sizeof(p));
  if (!ff_unpack(p) || p.cmd != FF_CMD_ACK)
    return;
  if (memcmp(info->src_addr, peer_mac, 6) != 0)
    return;

  comm._last_ack_ms = millis();
  if (info->rx_ctrl)
    comm._last_rssi = info->rx_ctrl->rssi;

  if (p.value == feed_seq && comm._feed_active) {
    comm._feed_result = FEED_OK;
    comm._feed_active = false;
    Serial.println("[COMM] feed ACK received");
  }
}
