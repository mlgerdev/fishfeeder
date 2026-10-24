/**
 * FishFeeder UI node — display HAL
 * Brings up: I2C bus -> XL9555 expander -> GT911 reset -> RGB panel -> LVGL.
 */
#pragma once
#include <lvgl.h>

// Initialize everything. Returns false on a fatal display error.
// (Touch is optional — the UI still comes up if GT911 is not found.)
bool display_init();

// Pump the LVGL timer handler — call as often as possible from loop().
void display_loop();
