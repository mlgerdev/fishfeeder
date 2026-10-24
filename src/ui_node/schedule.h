/**
 * FishFeeder UI node — feeding schedule engine
 * 4 daily slots, persisted in NVS, with a persisted feed-history ring buffer.
 */
#pragma once
#include <Arduino.h>
#include <Preferences.h>

#define FEED_SLOTS 4
#define HISTORY_LEN 12

struct FeedSlot {
  uint8_t hour;
  uint8_t minute;
  uint16_t duration_s; // relay pulse length in seconds
  bool enabled;
};

struct FeedEvent {
  uint32_t epoch;      // UTC epoch of the feed
  uint16_t duration_s; // pulse length
  uint8_t source;      // ff_source (manual / schedule)
};

class Schedule {
public:
  void begin(); // opens NVS and loads state

  FeedSlot slots[FEED_SLOTS] = {{7, 30, 3, true},
                                {12, 0, 3, true},
                                {18, 0, 3, true},
                                {0, 0, 3, false}};

  // Call once per second with LOCAL time. Returns the slot index that must
  // fire now (fires exactly once per slot per day), or -1.
  int tick(const struct tm &lt);

  // Next upcoming enabled slot: seconds-from-now (or -1) and its index.
  int32_t nextFeedInSeconds(const struct tm &lt, int &slot_idx) const;

  void save(); // persist slots

  // --- feed history (newest first, persisted) ---
  void recordEvent(uint32_t epoch, uint16_t duration_s, uint8_t source);
  int historyCount() const { return _hist_count; }
  const FeedEvent *history(int i) const { return &_hist[i]; }

private:
  void load();
  uint16_t _fired_day[FEED_SLOTS] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  FeedEvent _hist[HISTORY_LEN] = {};
  int _hist_count = 0;
  Preferences _prefs;
};
