#include "schedule.h"

static const char *PREF_NS = "fishfeeder";

void Schedule::begin() {
  _prefs.begin(PREF_NS, false);
  load();
}

void Schedule::load() {
  if (_prefs.getBytesLength("slots") == sizeof(slots))
    _prefs.getBytes("slots", slots, sizeof(slots));

  // sanity check — restore defaults if NVS holds garbage
  bool bad = false;
  for (auto &s : slots)
    if (s.hour > 23 || s.minute > 59 || s.duration_s == 0 || s.duration_s > 90)
      bad = true;
  if (bad) {
    slots[0] = {7, 30, 3, true};
    slots[1] = {12, 0, 3, true};
    slots[2] = {18, 0, 3, true};
    slots[3] = {0, 0, 3, false};
    save();
  }

  int len = _prefs.getBytesLength("hist");
  if (len > (int)sizeof(_hist))
    len = sizeof(_hist);
  _hist_count = len / sizeof(FeedEvent);
  if (_hist_count > 0)
    _prefs.getBytes("hist", _hist, _hist_count * sizeof(FeedEvent));

  if (_prefs.getBytesLength("fired") == sizeof(_fired_day))
    _prefs.getBytes("fired", _fired_day, sizeof(_fired_day));
}

void Schedule::save() { _prefs.putBytes("slots", slots, sizeof(slots)); }

int Schedule::tick(const struct tm &lt) {
  uint16_t now_min = lt.tm_hour * 60 + lt.tm_min;
  uint16_t today = (uint16_t)lt.tm_yday;

  for (int i = 0; i < FEED_SLOTS; i++) {
    const FeedSlot &s = slots[i];
    if (!s.enabled)
      continue;
    uint16_t slot_min = s.hour * 60 + s.minute;
    if (slot_min == now_min && _fired_day[i] != today) {
      _fired_day[i] = today; // don't double-fire within the same day
      _prefs.putBytes("fired", _fired_day, sizeof(_fired_day));
      return i;
    }
  }
  return -1;
}

int32_t Schedule::nextFeedInSeconds(const struct tm &lt, int &slot_idx) const {
  int32_t now_s = lt.tm_hour * 3600L + lt.tm_min * 60L + lt.tm_sec;
  int32_t best = -1;
  slot_idx = -1;
  for (int i = 0; i < FEED_SLOTS; i++) {
    if (!slots[i].enabled)
      continue;
    int32_t t = slots[i].hour * 3600L + slots[i].minute * 60L;
    int32_t dt = t - now_s;
    if (dt <= 0)
      dt += 86400L;
    if (best < 0 || dt < best) {
      best = dt;
      slot_idx = i;
    }
  }
  return best;
}

void Schedule::recordEvent(uint32_t epoch, uint16_t duration_s,
                           uint8_t source) {
  // newest-first ring buffer
  int last = (_hist_count < HISTORY_LEN) ? _hist_count : HISTORY_LEN - 1;
  for (int i = last; i > 0; i--)
    _hist[i] = _hist[i - 1];
  if (_hist_count < HISTORY_LEN)
    _hist_count++;
  _hist[0] = {epoch, duration_s, source};
  _prefs.putBytes("hist", _hist, _hist_count * sizeof(FeedEvent));
}
