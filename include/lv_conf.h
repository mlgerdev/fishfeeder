/**
 * FishFeeder — LVGL 9.2 configuration.
 * Minimal overrides only; everything else falls back to lv_conf_internal.h
 * defaults. Enabled via -DLV_CONF_INCLUDE_SIMPLE in platformio.ini.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* 16-bit RGB565 framebuffer (matches the ST7262 RGB panel) */
#define LV_COLOR_DEPTH 16

/* LVGL built-in allocator with a fixed pool — avoids heap fragmentation */
#define LV_USE_STDLIB_MALLOC 0
#define LV_MEM_SIZE (192 * 1024U)

/* Drive lv_tick from Arduino millis() */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_DEF_REFR_PERIOD 33 /* ~30 FPS */

/* Fonts used by the FishFeeder UI (Montserrat) */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_40 1

/* Keep the binary lean */
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0

#endif /* LV_CONF_H */
