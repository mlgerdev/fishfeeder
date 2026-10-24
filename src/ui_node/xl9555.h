/**
 * Driver for the XL9555 I2C GPIO expander on the Waveshare
 * ESP32-S3-Touch-LCD-4.3B (backlight, touch reset, SD CS live here).
 * Register-compatible with the TI TCA9555. Port0 = bits 0..7, Port1 = 8..15.
 */
#pragma once
#include <Arduino.h>
#include <Wire.h>

class XL9555 {
public:
  // addr is the 7-bit I2C address (0x20 when A0..A2 are tied low)
  bool begin(TwoWire &wire, uint8_t addr = 0x20);

  // Make the bits of `outputMask` outputs (bit = 1 -> output). Returns success.
  bool setOutputs(uint16_t outputMask);

  // Drive all pins: bit = 1 -> high, 0 -> low (outputs only).
  bool write(uint16_t value);

  // Read all input levels.
  uint16_t read();

private:
  bool writeReg(uint8_t reg, uint8_t val);
  TwoWire *_wire = nullptr;
  uint8_t _addr = 0x20;
};
