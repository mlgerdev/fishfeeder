/**
 * Waveshare ESP32-S3-Touch-LCD-4.3B — pin map
 * Source: https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3B
 *
 * NOTE: EXIO* pins live on the onboard XL9555 I2C GPIO expander (addr 0x20,
 * TCA9555-compatible) — see xl9555.h. The expander shares the I2C bus with
 * the GT911 touch controller (GPIO8/GPIO9).
 */
#pragma once

// ---- RGB LCD (16-lane RGB565, ST7262 800x480) ----
// data lane order for esp_lcd: DATA0..DATA15 = B3..B7, G2..G7, R3..R7
#define PIN_LCD_VSYNC 3
#define PIN_LCD_HSYNC 46
#define PIN_LCD_DE 5
#define PIN_LCD_PCLK 7

#define PIN_LCD_B3 14
#define PIN_LCD_B4 38
#define PIN_LCD_B5 18
#define PIN_LCD_B6 17
#define PIN_LCD_B7 10
#define PIN_LCD_G2 39
#define PIN_LCD_G3 0
#define PIN_LCD_G4 45
#define PIN_LCD_G5 48
#define PIN_LCD_G6 47
#define PIN_LCD_G7 21
#define PIN_LCD_R3 1
#define PIN_LCD_R4 2
#define PIN_LCD_R5 42
#define PIN_LCD_R6 41
#define PIN_LCD_R7 40

// ---- Touch (GT911 over I2C) ----
#define PIN_TP_SDA 8
#define PIN_TP_SCL 9
#define PIN_TP_IRQ 4
// TP_RST is on the XL9555 expander (EXIO1) — handled in hal_display.cpp

// ---- XL9555 expander bit positions (Port 0) ----
#define EXIO1_TP_RST 1 // touch controller reset
#define EXIO2_DISP 2   // LCD backlight / display enable
#define EXIO4_SD_CS 4  // reserved — SD card CS (not used in v1)

// ---- Panel timing (ST7262 800x480, ~60 Hz class timings) ----
#define LCD_H_RES 800
#define LCD_V_RES 480
#define LCD_PCLK_HZ (16 * 1000 * 1000)
#define LCD_HSW 4
#define LCD_HBP 43
#define LCD_HFP 8
#define LCD_VSW 10
#define LCD_VBP 12
#define LCD_VFP 8

// ---- Touch orientation fixes (applied in the LVGL read callback) ----
#define TOUCH_SWAP_XY 0
#define TOUCH_INVERT_X 1
#define TOUCH_INVERT_Y 0

// If the panel shows shifted/inverted colors, try flipping:
//   cfg.timings.flags.pclk_active_neg  (in hal_display.cpp)
// If the screen stays dark, check EXIO2_DISP polarity in hal_display.cpp.
