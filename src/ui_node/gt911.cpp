#include "gt911.h"

#define GT911_REG_STATUS 0x814E
#define GT911_REG_POINT1 0x8150

bool GT911::begin(TwoWire &wire) {
  _wire = &wire;
  const uint8_t addrs[2] = {0x5D, 0x14}; // ADDR pin high / low
  for (uint8_t a : addrs) {
    _wire->beginTransmission(a);
    if (_wire->endTransmission() == 0) {
      _addr = a;
      break;
    }
  }
  if (!_addr)
    return false;
  writeReg(GT911_REG_STATUS, 0x00); // clear any pending status
  return true;
}

bool GT911::writeReg(uint16_t reg, uint8_t val) {
  _wire->beginTransmission(_addr);
  _wire->write((uint8_t)(reg >> 8));
  _wire->write((uint8_t)(reg & 0xFF));
  _wire->write(val);
  return _wire->endTransmission() == 0;
}

bool GT911::readRegs(uint16_t reg, uint8_t *buf, uint8_t len) {
  _wire->beginTransmission(_addr);
  _wire->write((uint8_t)(reg >> 8));
  _wire->write((uint8_t)(reg & 0xFF));
  if (_wire->endTransmission() != 0)
    return false;
  if (_wire->requestFrom((int)_addr, (int)len) != len)
    return false;
  for (uint8_t i = 0; i < len; i++)
    buf[i] = _wire->read();
  return true;
}

bool GT911::readTouch(int16_t &x, int16_t &y) {
  if (!_addr)
    return false;

  bool touched = false;
  uint8_t status = 0;
  if (readRegs(GT911_REG_STATUS, &status, 1)) {
    // bit7 = buffer ready, bits 3..0 = number of touch points
    if ((status & 0x80) && ((status & 0x0F) > 0)) {
      uint8_t p[6]; // track id, xl, xh, yl, yh, size
      if (readRegs(GT911_REG_POINT1, p, 6)) {
        x = (int16_t)(((p[2] & 0x0F) << 8) | p[1]);
        y = (int16_t)(((p[4] & 0x0F) << 8) | p[3]);
        touched = true;
      }
    }
  }
  // Clear buffer-ready flag so the next sample can arrive
  writeReg(GT911_REG_STATUS, 0x00);
  return touched;
}
