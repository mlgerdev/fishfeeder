/**
 * Minimal GT911 capacitive touch driver (polling, first touch point only).
 * The Waveshare 4.3B wires GT911 to I2C0 (GPIO8/GPIO9); its reset line is on
 * the XL9555 expander (EXIO1) and is pulsed by hal_display.cpp before begin().
 */
#pragma once
#include <Arduino.h>
#include <Wire.h>

class GT911 {
public:
  bool begin(TwoWire &wire); // probes 0x5D then 0x14
  bool ok() const { return _addr != 0; }

  // Returns true while a finger is down; x/y in raw panel coordinates.
  bool readTouch(int16_t &x, int16_t &y);

private:
  bool writeReg(uint16_t reg, uint8_t val);
  bool readRegs(uint16_t reg, uint8_t *buf, uint8_t len);
  TwoWire *_wire = nullptr;
  uint8_t _addr = 0;
};
