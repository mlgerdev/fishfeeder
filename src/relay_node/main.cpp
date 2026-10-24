/**
 * FishFeeder relay node — ESP8266 NodeMCU V3 + 5V 1CH relay
 *
 * Receives ESP-NOW feed commands from the UI node (ESP32-S3 panel), ACKs them
 * and pulses the relay for the requested duration.
 *
 * Safety:
 *  - relay is forced OFF at boot and after every pulse
 *  - duration is clamped to [FF_FEED_DURATION_MIN_MS .. FF_FEED_DURATION_MAX_MS]
 *
 * Wiring (5V 1CH opto-isolated relay module):
 *   VCC -> 5V (Vin), GND -> GND, IN -> D1 (GPIO5)
 *   Most blue opto modules are ACTIVE-LOW; flip RELAY_ACTIVE_LEVEL if yours
 *   behaves inverted.
 *
 * First boot: open the serial monitor (115200) and copy the printed RELAY MAC
 * into src/ui_node/secrets.h.
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "shared/protocol.h"
#include "shared/config.h"
#include "secrets.h"

// --- hardware config ---
#define PIN_RELAY 5 // D1 on NodeMCU — safe, non-boot-critical pin
#define RELAY_ACTIVE_LEVEL LOW // most opto-isolated 5V relay boards are active-LOW
#define PIN_LED 2              // onboard LED, active LOW on most NodeMCU boards

#define RELAY_ON() digitalWrite(PIN_RELAY, RELAY_ACTIVE_LEVEL)
#define RELAY_OFF() digitalWrite(PIN_RELAY, !RELAY_ACTIVE_LEVEL)

static bool feeding = false;
static uint32_t feed_end_ms = 0;
static uint32_t last_wifi_try = 0;

static void sendAck(const uint8_t *mac, uint32_t echo_seq) {
  ff_packet p;
  ff_pack(p, FF_CMD_ACK, 0, echo_seq, 0);
  uint8_t ch = WiFi.channel();
  if (ch == 0)
    ch = FF_ESPNOW_CHANNEL;
  // ESP8266 requires the peer to exist before esp_now_send
  esp_now_add_peer((uint8_t *)mac, ESP_NOW_ROLE_COMBO, ch, NULL, 0);
  esp_now_send((uint8_t *)mac, (uint8_t *)&p, sizeof(p));
}

static void onRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(ff_packet))
    return;
  ff_packet p;
  memcpy(&p, data, sizeof(p));
  if (!ff_unpack(p))
    return;

  switch (p.cmd) {
  case FF_CMD_PING:
    sendAck(mac, p.seq);
    break;

  case FF_CMD_FEED: {
    uint32_t dur = p.value;
    if (dur < FF_FEED_DURATION_MIN_MS)
      dur = FF_FEED_DURATION_MIN_MS;
    if (dur > FF_FEED_DURATION_MAX_MS)
      dur = FF_FEED_DURATION_MAX_MS;
    sendAck(mac, p.seq);
    if (!feeding) { // ignore duplicates while a pulse is running
      feeding = true;
      feed_end_ms = millis() + dur;
      RELAY_ON();
      Serial.printf("[FEED] relay ON for %lu ms (source %u)\n",
                    (unsigned long)dur, p.source);
    }
    break;
  }

  default:
    break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(PIN_RELAY, OUTPUT);
  RELAY_OFF(); // safety first: feeder off at boot
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW); // LED on while booting

  Serial.println("\n=== FishFeeder relay node (ESP8266) ===");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // keep the radio awake for ESP-NOW RX
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] connecting to \"%s\"...\n", WIFI_SSID);

  if (esp_now_init() != 0) {
    Serial.println("[ESP-NOW] init FAILED");
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onRecv);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.println("+--------------------------------------------------+");
  Serial.printf("|  RELAY MAC: %02X:%02X:%02X:%02X:%02X:%02X              |\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("|  Put this MAC into src/ui_node/secrets.h (RELAY_MAC) |");
  Serial.println("+--------------------------------------------------+");
  digitalWrite(PIN_LED, HIGH);
}

void loop() {
  // keep WiFi (and therefore the ESP-NOW channel) alive
  if (WiFi.status() != WL_CONNECTED && millis() - last_wifi_try > 10000) {
    last_wifi_try = millis();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.println("[WiFi] reconnecting...");
  }

  // feed pulse end / LED feedback
  if (feeding) {
    digitalWrite(PIN_LED, (millis() / 100) & 1 ? LOW : HIGH); // blink fast
    if ((int32_t)(millis() - feed_end_ms) >= 0) {
      feeding = false;
      RELAY_OFF();
      digitalWrite(PIN_LED, HIGH);
      Serial.println("[FEED] done, relay OFF");
    }
  }

  delay(2);
}
