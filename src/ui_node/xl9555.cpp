#include "xl9555.h"

// TCA9555 / XL9555 register map
#define XL_REG_IN0 0x00
#define XL_REG_IN1 0x01
#define XL_REG_OUT0 0x02
#define XL_REG_OUT1 0x03
#define XL_REG_CFG0 0x06
#define XL_REG_CFG1 0x07

bool XL9555::begin(TwoWire &wire, uint8_t addr) {
  _wire = &wire;
  _addr = addr;
  _wire->beginTransmission(_addr);
  if (_wire->endTransmission() != 0)
    return false;
  // Power-up default: all pins inputs (config = 0xFFFF), outputs latched high.
  // Park outputs high first so enabling them never glitches anything low.
  writeReg(XL_REG_OUT0, 0xFF);
  writeReg(XL_REG_OUT1, 0xFF);
  return true;
}

bool XL9555::writeReg(uint8_t reg, uint8_t val) {
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->write(val);
  return _wire->endTransmission() == 0;
}

bool XL9555::setOutputs(uint16_t outputMask) {
  // config register: 0 = output, 1 = input
  uint8_t cfg0 = 0xFF & ~(outputMask & 0xFF);
  uint8_t cfg1 = 0xFF & ~((outputMask >> 8) & 0xFF);
  return writeReg(XL_REG_CFG0, cfg0) && writeReg(XL_REG_CFG1, cfg1);
}

bool XL9555::write(uint16_t value) {
  return writeReg(XL_REG_OUT0, value & 0xFF) &&
         writeReg(XL_REG_OUT1, (value >> 8) & 0xFF);
}

uint16_t XL9555::read() {
  if (!_wire)
    return 0xFFFF;
  _wire->beginTransmission(_addr);
  _wire->write(XL_REG_IN0);
  if (_wire->endTransmission() != 0)
    return 0xFFFF;
  if (_wire->requestFrom((int)_addr, 2) != 2)
    return 0xFFFF;
  uint8_t lo = _wire->read();
  uint8_t hi = _wire->read();
  return (uint16_t)(lo | (hi << 8));
}
