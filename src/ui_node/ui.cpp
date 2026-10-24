/**
 * FishFeeder UI node — LVGL 9 screens (800x480, dark aqua theme).
 * Three screens + bottom nav: Home, Schedule (4 slots + editor), Status.
 * Design tokens mirror figma/design-spec.md so Figma -> LVGL Flow maps 1:1.
 */
#include "ui.h"
#include <Arduino.h>
#include <time.h>
#include "hal_display.h"
#include "schedule.h"
#include "comm.h"
#include "shared/config.h"

extern Schedule sched;

// ---- design tokens (figma/design-spec.md) ----
#define C_BG lv_color_hex(0x0B1220)
#define C_CARD lv_color_hex(0x13223D)
#define C_CARD_HI lv_color_hex(0x1B3054)
#define C_ACCENT lv_color_hex(0x00C2A8)
#define C_ACCENT_DIM lv_color_hex(0x0A5F55)
#define C_WARN lv_color_hex(0xFFB454)
#define C_DANGER lv_color_hex(0xFF6B6B)
#define C_TEXT lv_color_hex(0xE6F1FF)
#define C_TEXT_DIM lv_color_hex(0x7C93B3)

#define F_BODY &lv_font_montserrat_16
#define F_SMALL &lv_font_montserrat_14
#define F_H2 &lv_font_montserrat_20
#define F_H1 &lv_font_montserrat_28
#define F_HUGE &lv_font_montserrat_40

// ---- widgets ----
static lv_obj_t *scr_home, *scr_sched, *scr_status;
static lv_obj_t *lbl_clock, *lbl_wifi, *lbl_relay;
static lv_obj_t *lbl_next_time, *lbl_next_in, *lbl_today, *btn_feed, *lbl_feed;
static lv_obj_t *slot_lbl[FEED_SLOTS];
static lv_obj_t *slot_sw[FEED_SLOTS];
static lv_obj_t *edit_overlay;
static int edit_idx = -1;
static lv_obj_t *roller_h, *roller_m, *spin_dur, *sw_en, *lbl_edit_title, *toast;
static lv_obj_t *hist_list, *lbl_ip, *lbl_rssi, *lbl_uptime, *lbl_fw,
    *lbl_relay_mac, *lbl_reen, *lbl_clock2;

static Schedule *sched_ptr = nullptr;
static char hour_opts[24 * 3 + 4], minute_opts[60 * 3 + 4];

// ---------------------------------------------------------------- helpers
static lv_obj_t *make_card(lv_obj_t *parent) {
  lv_obj_t *c = lv_obj_create(parent);
  lv_obj_set_style_bg_color(c, C_CARD, 0);
  lv_obj_set_style_border_width(c, 0, 0);
  lv_obj_set_style_radius(c, 14, 0);
  lv_obj_set_style_pad_all(c, 14, 0);
  lv_obj_set_style_shadow_width(c, 0, 0);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  return c;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *txt,
                            const lv_font_t *font, lv_color_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  return l;
}

static void nav_cb(lv_event_t *e) {
  lv_obj_t *scr = (lv_obj_t *)lv_event_get_user_data(e);
  lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 150, 0, false);
}

static void build_nav(lv_obj_t *parent) {
  lv_obj_t *bar = lv_obj_create(parent);
  lv_obj_set_size(bar, LV_PCT(100), 68);
  lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(bar, C_CARD, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  const char *names[3] = {LV_SYMBOL_HOME "  Home",
                          LV_SYMBOL_SETTINGS "  Schedule",
                          LV_SYMBOL_LIST "  Status"};
  lv_obj_t *targets[3] = {scr_home, scr_sched, scr_status};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = lv_button_create(bar);
    lv_obj_set_size(b, 220, 46);
    lv_obj_align(b, LV_ALIGN_LEFT_MID, 30 + i * 250, 0);
    lv_obj_set_style_bg_color(b, C_ACCENT_DIM, 0);
    lv_obj_add_event_cb(b, nav_cb, LV_EVENT_CLICKED, targets[i]);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, names[i]);
    lv_obj_set_style_text_font(l, F_BODY, 0);
    lv_obj_center(l);
  }
}

static void header_init(lv_obj_t *scr, lv_obj_t **clock_lbl) {
  lv_obj_t *hdr = lv_obj_create(scr);
  lv_obj_set_size(hdr, LV_PCT(100), 60);
  lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(hdr, 0, 0);
  lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

  *clock_lbl = make_label(hdr, "--:--", F_H1, C_TEXT);
  lv_obj_align(*clock_lbl, LV_ALIGN_LEFT_MID, 8, 0);

  lbl_wifi = make_label(hdr, "WiFi ...", F_SMALL, C_TEXT_DIM);
  lv_obj_align(lbl_wifi, LV_ALIGN_RIGHT_MID, -150, 0);

  lbl_relay = make_label(hdr, "Relay ...", F_SMALL, C_TEXT_DIM);
  lv_obj_align(lbl_relay, LV_ALIGN_RIGHT_MID, -8, 0);
}

// ---------------------------------------------------------------- home screen
static void feed_now_cb(lv_event_t *e) {
  (void)e;
  if (!comm.feedInFlight())
    comm.sendFeed(FF_FEED_DURATION_DEFAULT_MS / 1000, FF_SRC_MANUAL);
}

static void build_home() {
  scr_home = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_home, C_BG, 0);
  lv_obj_set_style_pad_all(scr_home, 0, 0);
  header_init(scr_home, &lbl_clock);

  // next-feed card
  lv_obj_t *card = make_card(scr_home);
  lv_obj_set_size(card, 360, 226);
  lv_obj_align(card, LV_ALIGN_TOP_LEFT, 16, 72);

  make_label(card, "NEXT FEED", F_SMALL, C_TEXT_DIM);
  lbl_next_time = make_label(card, "--:--", F_HUGE, C_TEXT);
  lv_obj_align(lbl_next_time, LV_ALIGN_TOP_MID, 0, 40);
  lbl_next_in = make_label(card, "waiting for time sync", F_BODY, C_TEXT_DIM);
  lv_obj_align(lbl_next_in, LV_ALIGN_BOTTOM_MID, 0, -4);

  // feed-now card
  lv_obj_t *card2 = make_card(scr_home);
  lv_obj_set_size(card2, 360, 226);
  lv_obj_align(card2, LV_ALIGN_TOP_RIGHT, -16, 72);

  btn_feed = lv_button_create(card2);
  lv_obj_set_size(btn_feed, 300, 110);
  lv_obj_align(btn_feed, LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_set_style_bg_color(btn_feed, C_ACCENT, 0);
  lv_obj_add_event_cb(btn_feed, feed_now_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(btn_feed);
  lv_label_set_text(bl, "FEED NOW");
  lv_obj_set_style_text_font(bl, F_HUGE, 0);
  lv_obj_set_style_text_color(bl, lv_color_hex(0x04211D), 0);
  lv_obj_center(bl);

  lbl_feed = make_label(card2, "Ready.", F_BODY, C_TEXT_DIM);
  lv_obj_align(lbl_feed, LV_ALIGN_BOTTOM_MID, 0, -8);

  // today card
  lv_obj_t *card3 = make_card(scr_home);
  lv_obj_set_size(card3, LV_PCT(100), 84);
  lv_obj_align(card3, LV_ALIGN_BOTTOM_MID, 0, -92);
  lbl_today = make_label(card3, "Today: 0 feeds", F_H2, C_TEXT);
  lv_obj_align(lbl_today, LV_ALIGN_LEFT_MID, 8, 0);

  build_nav(scr_home);
}

// ---------------------------------------------------------------- schedule screen
static void toast_timer_cb(lv_timer_t *t) {
  lv_obj_add_flag((lv_obj_t *)t->user_data, LV_OBJ_FLAG_HIDDEN);
  lv_timer_del(t);
}

static void show_toast(const char *txt) {
  if (!toast)
    return;
  lv_label_set_text(lv_obj_get_child(toast, 0), txt);
  lv_obj_remove_flag(toast, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(toast);
  lv_timer_t *t = lv_timer_create(toast_timer_cb, 2200, toast);
  (void)t;
}

static void slot_label_refresh(int i) {
  const FeedSlot &s = sched_ptr->slots[i];
  lv_label_set_text_fmt(slot_lbl[i], "%02d:%02d   -   %us", s.hour, s.minute,
                        s.duration_s);
  lv_obj_set_style_text_color(slot_lbl[i],
                              s.enabled ? C_TEXT : C_TEXT_DIM, 0);
}

static void slot_sw_cb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  sched_ptr->slots[i].enabled =
      lv_obj_has_state(slot_sw[i], LV_STATE_CHECKED);
  sched_ptr->save();
  slot_label_refresh(i);
}

static void slot_row_cb(lv_event_t *e) {
  edit_idx = (int)(intptr_t)lv_event_get_user_data(e);
  const FeedSlot &s = sched_ptr->slots[edit_idx];
  lv_roller_set_selected_row(roller_h, s.hour);
  lv_roller_set_selected_row(roller_m, s.minute);
  lv_spinbox_set_value(spin_dur, s.duration_s);
  if (s.enabled)
    lv_obj_add_state(sw_en, LV_STATE_CHECKED);
  else
    lv_obj_remove_state(sw_en, LV_STATE_CHECKED);
  lv_label_set_text_fmt(lbl_edit_title, "Edit slot %d", edit_idx + 1);
  lv_obj_remove_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(edit_overlay);
}

static void editor_save_cb(lv_event_t *e) {
  (void)e;
  if (edit_idx >= 0) {
    FeedSlot &s = sched_ptr->slots[edit_idx];
    s.hour = lv_roller_get_selected(roller_h);
    s.minute = lv_roller_get_selected(roller_m);
    s.duration_s = (uint16_t)lv_spinbox_get_value(spin_dur);
    s.enabled = lv_obj_has_state(sw_en, LV_STATE_CHECKED);
    sched_ptr->save();
    slot_label_refresh(edit_idx);
  }
  lv_obj_add_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
  show_toast("Schedule saved");
}

static void editor_cancel_cb(lv_event_t *e) {
  (void)e;
  lv_obj_add_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void build_schedule() {
  scr_sched = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_sched, C_BG, 0);
  lv_obj_set_style_pad_all(scr_sched, 0, 0);
  make_label(scr_sched, "Daily feed schedule", F_H1, C_TEXT);
  lbl_clock2 = make_label(scr_sched, "--:--", F_H1, C_TEXT);
  lv_obj_align(lbl_clock2, LV_ALIGN_TOP_RIGHT, -16, 0);

  for (int i = 0; i < FEED_SLOTS; i++) {
    lv_obj_t *row = make_card(scr_sched);
    lv_obj_set_size(row, LV_PCT(100), 74);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 16, 52 + i * 84);
    lv_obj_set_style_bg_color(row, C_CARD_HI, 0);
    lv_obj_add_event_cb(row, slot_row_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_align(sw, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_add_event_cb(sw, slot_sw_cb, LV_EVENT_VALUE_CHANGED,
                        (void *)(intptr_t)i);
    slot_sw[i] = sw;

    lv_obj_t *lbl = make_label(row, "--:--", F_H2, C_TEXT);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 110, 0);
    slot_lbl[i] = lbl;

    make_label(row, "tap to edit", F_SMALL, C_TEXT_DIM);
    lv_obj_align(lv_obj_get_child(row, -1), LV_ALIGN_RIGHT_MID, -8, 0);

    slot_label_refresh(i);
  }

  build_nav(scr_sched);

  // ---- editor overlay (hidden by default) ----
  edit_overlay = lv_obj_create(scr_sched);
  lv_obj_set_size(edit_overlay, 560, 400);
  lv_obj_center(edit_overlay);
  lv_obj_set_style_bg_color(edit_overlay, C_CARD_HI, 0);
  lv_obj_set_style_border_color(edit_overlay, C_ACCENT_DIM, 0);
  lv_obj_set_style_border_width(edit_overlay, 2, 0);
  lv_obj_set_style_radius(edit_overlay, 18, 0);
  lv_obj_add_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(edit_overlay, LV_OBJ_FLAG_SCROLLABLE);

  lbl_edit_title = make_label(edit_overlay, "Edit slot", F_H1, C_TEXT);
  lv_obj_align(lbl_edit_title, LV_ALIGN_TOP_LEFT, 8, 4);

  sw_en = lv_switch_create(edit_overlay);
  lv_obj_align(sw_en, LV_ALIGN_TOP_RIGHT, -8, 4);

  roller_h = lv_roller_create(edit_overlay);
  lv_roller_set_options(roller_h, hour_opts, LV_ROLLER_MODE_NORMAL);
  lv_obj_set_size(roller_h, 110, 180);
  lv_obj_align(roller_h, LV_ALIGN_TOP_LEFT, 40, 60);
  lv_obj_set_style_bg_color(roller_h, C_CARD, 0);

  roller_m = lv_roller_create(edit_overlay);
  lv_roller_set_options(roller_m, minute_opts, LV_ROLLER_MODE_NORMAL);
  lv_obj_set_size(roller_m, 110, 180);
  lv_obj_align(roller_m, LV_ALIGN_TOP_LEFT, 170, 60);
  lv_obj_set_style_bg_color(roller_m, C_CARD, 0);

  make_label(edit_overlay, "h", F_H2, C_TEXT_DIM);
  lv_obj_align(lv_obj_get_child(edit_overlay, -1), LV_ALIGN_TOP_LEFT, 85, 250);
  make_label(edit_overlay, "min", F_H2, C_TEXT_DIM);
  lv_obj_align(lv_obj_get_child(edit_overlay, -1), LV_ALIGN_TOP_LEFT, 205, 250);

  spin_dur = lv_spinbox_create(edit_overlay);
  lv_spinbox_set_range(spin_dur, 1, 90);
  lv_spinbox_set_digit_format(spin_dur, 2, 0);
  lv_spinbox_set_step(spin_dur, 1);
  lv_obj_set_size(spin_dur, 120, 56);
  lv_obj_align(spin_dur, LV_ALIGN_TOP_RIGHT, -40, 70);
  lv_obj_set_style_bg_color(spin_dur, C_CARD, 0);

  make_label(edit_overlay, "seconds", F_H2, C_TEXT_DIM);
  lv_obj_align(lv_obj_get_child(edit_overlay, -1), LV_ALIGN_TOP_RIGHT, -40,
               135);

  lv_obj_t *b_save = lv_button_create(edit_overlay);
  lv_obj_set_size(b_save, 150, 56);
  lv_obj_align(b_save, LV_ALIGN_BOTTOM_LEFT, 90, -16);
  lv_obj_set_style_bg_color(b_save, C_ACCENT, 0);
  lv_obj_add_event_cb(b_save, editor_save_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *sl = lv_label_create(b_save);
  lv_label_set_text(sl, "Save");
  lv_obj_center(sl);

  lv_obj_t *b_cancel = lv_button_create(edit_overlay);
  lv_obj_set_size(b_cancel, 150, 56);
  lv_obj_align(b_cancel, LV_ALIGN_BOTTOM_RIGHT, -90, -16);
  lv_obj_set_style_bg_color(b_cancel, C_CARD, 0);
  lv_obj_add_event_cb(b_cancel, editor_cancel_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *cl = lv_label_create(b_cancel);
  lv_label_set_text(cl, "Cancel");
  lv_obj_center(cl);

  // toast (child of schedule screen, shown after save)
  toast = lv_obj_create(scr_sched);
  lv_obj_set_size(toast, 300, 64);
  lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -84);
  lv_obj_set_style_bg_color(toast, C_ACCENT_DIM, 0);
  lv_obj_set_style_radius(toast, 12, 0);
  lv_obj_set_style_border_width(toast, 0, 0);
  lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(toast, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *tl = lv_label_create(toast);
  lv_label_set_text(tl, "Saved");
  lv_obj_set_style_text_color(tl, C_TEXT, 0);
  lv_obj_center(tl);
}

// ---------------------------------------------------------------- status screen
static void hist_refresh() {
  if (!hist_list)
    return;
  lv_obj_clean(hist_list);
  int n = sched_ptr->historyCount();
  if (n == 0) {
    lv_obj_t *l = make_label(hist_list, "No feeds recorded yet.", F_BODY,
                             C_TEXT_DIM);
    lv_obj_set_style_pad_left(l, 8, 0);
    return;
  }
  for (int i = 0; i < n; i++) {
    const FeedEvent *ev = sched_ptr->history(i);
    char tbuf[24] = "?";
    if (ev->epoch > 1700000000) {
      time_t et = (time_t)ev->epoch;
      struct tm etm;
      localtime_r(&et, &etm);
      snprintf(tbuf, sizeof(tbuf), "%02d-%02d %02d:%02d", etm.tm_mon + 1,
               etm.tm_mday, etm.tm_hour, etm.tm_min);
    }
    char line[96];
    snprintf(line, sizeof(line), "%s   %us   %s", tbuf, ev->duration_s,
             ev->source == FF_SRC_MANUAL ? "manual" : "schedule");
    lv_obj_t *l = make_label(hist_list, line, F_BODY,
                             i == 0 ? C_TEXT : C_TEXT_DIM);
    lv_obj_set_style_pad_left(l, 8, 0);
  }
}

static void build_status() {
  scr_status = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_status, C_BG, 0);
  lv_obj_set_style_pad_all(scr_status, 0, 0);
  make_label(scr_status, "System status", F_H1, C_TEXT);

  lv_obj_t *card = make_card(scr_status);
  lv_obj_set_size(card, 380, 160);
  lv_obj_align(card, LV_ALIGN_TOP_LEFT, 16, 72);

  lbl_ip = make_label(card, "IP: -", F_BODY, C_TEXT);
  lbl_rssi = make_label(card, "WiFi RSSI: -", F_BODY, C_TEXT);
  lbl_uptime = make_label(card, "Uptime: -", F_BODY, C_TEXT);
  lbl_fw = make_label(card, "FW: 1.0.0", F_BODY, C_TEXT);
  lv_obj_align_to(lbl_ip, card, LV_ALIGN_TOP_LEFT, 8, 8);
  lv_obj_align_to(lbl_rssi, lbl_ip, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);
  lv_obj_align_to(lbl_uptime, lbl_rssi, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);
  lv_obj_align_to(lbl_fw, lbl_uptime, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);

  lv_obj_t *card2 = make_card(scr_status);
  lv_obj_set_size(card2, 380, 160);
  lv_obj_align(card2, LV_ALIGN_TOP_RIGHT, -16, 72);
  lbl_relay_mac = make_label(card2, "Relay: -", F_BODY, C_TEXT);
  lbl_reen = make_label(card2, "Link RSSI: -", F_BODY, C_TEXT);
  lv_obj_align_to(lbl_relay_mac, card2, LV_ALIGN_TOP_LEFT, 8, 8);
  lv_obj_align_to(lbl_reen, lbl_relay_mac, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);

  make_label(scr_status, "Recent feeds", F_H2, C_TEXT);
  lv_obj_align(lv_obj_get_child(scr_status, -1), LV_ALIGN_TOP_LEFT, 16, 244);

  hist_list = lv_obj_create(scr_status);
  lv_obj_set_size(hist_list, LV_PCT(100), 128);
  lv_obj_align(hist_list, LV_ALIGN_BOTTOM_MID, 0, -84);
  lv_obj_set_style_bg_color(hist_list, C_CARD, 0);
  lv_obj_set_style_border_width(hist_list, 0, 0);
  lv_obj_set_style_radius(hist_list, 14, 0);
  lv_obj_set_flex_flow(hist_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(hist_list, 8, 0);
  lv_obj_scroll_to_y(hist_list, 0, LV_ANIM_OFF);

  build_nav(scr_status);
  hist_refresh();
}

// ---------------------------------------------------------------- init / tick
void ui_init(Schedule *s) {
  sched_ptr = s;

  // roller option strings: "00\n01\n...23"
  char *p = hour_opts;
  for (int i = 0; i < 24; i++)
    p += snprintf(p, 4, "%02d\n", i);
  p[-1] = '\0';
  p = minute_opts;
  for (int i = 0; i < 60; i++)
    p += snprintf(p, 4, "%02d\n", i);
  p[-1] = '\0';

  build_home();
  build_schedule();
  build_status();
  lv_screen_load_anim(scr_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void ui_tick() {
  // clock
  time_t now_t;
  time(&now_t);
  struct tm lt;
  localtime_r(&now_t, &lt);
  char cbuf[8];
  if (comm.timeValid())
    snprintf(cbuf, sizeof(cbuf), "%02d:%02d", lt.tm_hour, lt.tm_min);
  else
    snprintf(cbuf, sizeof(cbuf), "--:--");
  lv_label_set_text(lbl_clock, cbuf);
  lv_label_set_text(lbl_clock2, cbuf);

  // wifi / relay header chips
  if (comm.wifiConnected()) {
    lv_label_set_text_fmt(lbl_wifi, "WiFi %d dBm", comm.wifiRssi());
    lv_obj_set_style_text_color(lbl_wifi, C_TEXT, 0);
  } else {
    lv_label_set_text(lbl_wifi, "WiFi offline");
    lv_obj_set_style_text_color(lbl_wifi, C_DANGER, 0);
  }
  if (comm.relayOnline()) {
    lv_label_set_text(lbl_relay, "Relay online");
    lv_obj_set_style_text_color(lbl_relay, C_ACCENT, 0);
  } else {
    lv_label_set_text(lbl_relay, "Relay offline");
    lv_obj_set_style_text_color(lbl_relay, C_WARN, 0);
  }

  // next feed
  int idx = -1;
  int32_t dt = sched_ptr->nextFeedInSeconds(lt, idx);
  if (idx >= 0) {
    const FeedSlot &s = sched_ptr->slots[idx];
    lv_label_set_text_fmt(lbl_next_time, "%02d:%02d", s.hour, s.minute);
    char ib[48];
    snprintf(ib, sizeof(ib), "in %ld h %ld m", (long)(dt / 3600),
             (long)((dt % 3600) / 60));
    lv_label_set_text(lbl_next_in, ib);
  } else {
    lv_label_set_text(lbl_next_time, "--:--");
    lv_label_set_text(lbl_next_in, "no feeds scheduled");
  }

  // feeds today
  int today_count = 0;
  uint16_t today = (uint16_t)lt.tm_yday;
  int n = sched_ptr->historyCount();
  for (int i = 0; i < n; i++) {
    const FeedEvent *ev = sched_ptr->history(i);
    if (ev->epoch > 1700000000) {
      time_t et = (time_t)ev->epoch;
      struct tm etm;
      localtime_r(&et, &etm);
      if ((uint16_t)etm.tm_yday == today)
        today_count++;
    }
  }
  lv_label_set_text_fmt(lbl_today, "Today: %d feed%s", today_count,
                        today_count == 1 ? "" : "s");

  // feed command status line
  if (comm.feedInFlight()) {
    lv_label_set_text(lbl_feed, "Sending feed command...");
    lv_obj_set_style_text_color(lbl_feed, C_WARN, 0);
  } else {
    Comm::FeedResult r = comm.lastFeedResult();
    if (r == Comm::FEED_OK) {
      lv_label_set_text(lbl_feed, "Feed dispensed. Bon appetit!");
      lv_obj_set_style_text_color(lbl_feed, C_ACCENT, 0);
    } else if (r == Comm::FEED_FAIL) {
      lv_label_set_text(lbl_feed, "Feed FAILED - relay did not answer");
      lv_obj_set_style_text_color(lbl_feed, C_DANGER, 0);
    } else {
      lv_label_set_text(lbl_feed, "Ready.");
      lv_obj_set_style_text_color(lbl_feed, C_TEXT_DIM, 0);
    }
  }

  // status screen values
  lv_label_set_text_fmt(lbl_ip, "IP: %s", comm.ipStr().c_str());
  lv_label_set_text_fmt(lbl_rssi, "WiFi RSSI: %d dBm",
                        comm.wifiConnected() ? comm.wifiRssi() : 0);
  lv_label_set_text_fmt(lbl_uptime, "Uptime: %lu min",
                        (unsigned long)(millis() / 60000UL));
  lv_label_set_text_fmt(lbl_relay_mac, "Relay: %s", comm.relayMacStr());
  lv_label_set_text_fmt(lbl_reen, "Link RSSI: %d dBm",
                        comm.relayOnline() ? comm.lastRssi() : 0);

  // history list — refresh if the schedule recorded something new
  static int last_hist_count = -1;
  if (sched_ptr->historyCount() != last_hist_count) {
    last_hist_count = sched_ptr->historyCount();
    hist_refresh();
  }
}
