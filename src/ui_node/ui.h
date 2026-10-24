/**
 * FishFeeder UI node — LVGL screens API
 */
#pragma once
#include <lvgl.h>

class Schedule;

void ui_init(Schedule *s);
void ui_tick(); // 1 Hz housekeeping: clock, next-feed, link status, history
