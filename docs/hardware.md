# Hardware Guide

## Bill of materials

| Part | Role | Notes |
|------|------|-------|
| Waveshare ESP32-S3-Touch-LCD-4.3B | UI node (brain) | ESP32-S3-WROOM-1-**N16R8** (16 MB flash, 8 MB OPI PSRAM), 4.3" 800x480 ST7262 RGB panel, GT911 capacitive touch |
| ESP8266 NodeMCU V3 (CH340) | Relay node (actuator) | 2.4 GHz WiFi, ESP-NOW capable |
| 5V 1CH relay module (opto-isolated) | Switches the feeder motor | Active-**LOW** input on most blue modules |
| 5V USB power supplies (2x) | Power | One per node; relay coil draws ~70 mA |

## UI node — Waveshare ESP32-S3-Touch-LCD-4.3B pin usage

| Function | GPIO |
|----------|------|
| LCD VSYNC / HSYNC / DE / PCLK | 3 / 46 / 5 / 7 |
| LCD data B3..B7 | 14, 38, 18, 17, 10 |
| LCD data G2..G7 | 39, 0, 45, 48, 47, 21 |
| LCD data R3..R7 | 1, 2, 42, 41, 40 |
| Touch I2C SDA / SCL / IRQ | 8 / 9 / 4 |
| Backlight (DISP) | XL9555 expander EXIO2 (I2C 0x20) |
| Touch reset | XL9555 expander EXIO1 |
| SD card CS | XL9555 expander EXIO4 (unused in v1) |
| CAN TX/RX (unused v1) | 15 / 16 |
| RS485 RX/TX (unused v1) | 43 / 44 |
| USB D-/D+ | 19 / 20 (native USB-CDC serial) |

Full reference: <https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3B>

## Relay node wiring

```
 NodeMCU V3                5V 1CH relay module
 +----------+              +------------------+
 |      VIN |--- 5V ------>| VCC              |
 |      GND |--- GND ----->| GND              |
 |    D1/G5 |--- signal -->| IN               |
 +----------+              |  COM  NO  NC     |
                           +---o---o---o------+
                                |   |
        feeder motor / auger <--'   (switch the motor's +5V through COM->NO)
```

* **COM + NO** (normally open) in series with the feeder motor's positive wire.
* Relay coil is powered from the module's **VCC (5V)**, not from the NodeMCU
  3V3 pin — the coil needs real 5 V and ~70 mA.
* If your module is **active-HIGH**, flip `RELAY_ACTIVE_LEVEL` in
  `src/relay_node/main.cpp`.

## Power & safety

* Feed everything from a common 5 V rail **of at least 2 A** if the feeder
  motor shares the supply with the electronics.
* Keep fish-food dust out of the relay; consider a small enclosure.
* The relay node clamps every pulse to 60 s max (`FF_FEED_DURATION_MAX_MS`) and
  forces the relay OFF at boot — a crashed UI node can never leave the feeder
  running.

## Known quirks

* **GPIO0** is wired to LCD data line G3 — it is a boot-strap pin, but the
  panel does not drive it during reset, so normal flashing works. If flashing
  fails, hold **BOOT**, tap **RESET**, release **BOOT**.
* The 4.3B routes backlight/touch-reset/SD-CS through an **XL9555 I2C
  expander at 0x20** (TCA9555-compatible) — see `xl9555.h`.
* If colors look wrong on the panel, toggle
  `cfg.timings.flags.pclk_active_neg` in `src/ui_node/hal_display.cpp`.
* If the screen stays black, check the EXIO2 (DISP) polarity right after
  `esp_lcd_panel_init()` in the same file.
