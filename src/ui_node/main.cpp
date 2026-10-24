/**
 * FishFeeder UI node — Waveshare ESP32-S3-Touch-LCD-4.3B
 * LVGL 9 HMI + feeding scheduler + ESP-NOW link to the ESP8266 relay node.
 *
 * First run: copy secrets.example.h to secrets.h and fill in WiFi + relay MAC.
 */
#include <Arduino.h>
#include "hal_display.h"
#include "ui.h"
#include "schedule.h"
#include "comm.h"
#include "shared/config.h"
#include "shared/protocol.h"

Schedule sched;

static uint32_t last_1s = 0;
static Comm::FeedResult last_seen_result = Comm::FEED_NONE;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== FishFeeder UI node (ESP32-S3-Touch-LCD-4.3B) ===");

  if (!display_init()) {
    Serial.println("FATAL: display init failed — halting");
    while (true)
      delay(1000);
  }

  sched.begin();
  comm.begin();
  ui_init(&sched);
  ui_tick();
  Serial.println("[MAIN] ready");
}

void loop() {
  display_loop(); // lv_timer_handler
  comm.loop();    // heartbeat + feed retries

  // record history once a feed result lands
  Comm::FeedResult r = comm.lastFeedResult();
  if (r != last_seen_result) {
    last_seen_result = r;
    if (r == Comm::FEED_OK && comm.timeValid())
      sched.recordEvent((uint32_t)time(nullptr), comm.lastFeedDurationS(),
                        comm.lastFeedSource());
  }

  // 1 Hz housekeeping: scheduler + UI refresh
  uint32_t now = millis();
  if (now - last_1s >= 1000) {
    last_1s = now;

    time_t t;
    time(&t);
    struct tm lt;
    localtime_r(&t, &lt);

    if (comm.timeValid()) {
      int slot = sched.tick(lt);
      if (slot >= 0)
        comm.sendFeed(sched.slots[slot].duration_s, FF_SRC_SCHEDULE);
    }
    ui_tick();
  }

  delay(2);
}
