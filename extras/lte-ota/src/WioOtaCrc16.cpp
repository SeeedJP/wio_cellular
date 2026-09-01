#include "WioOtaCrc16.h"

namespace wio_ota {

uint16_t crc16Ccitt(const uint8_t* data, size_t size, uint16_t previous) {
  uint16_t crc = previous;

  for (size_t i = 0; i < size; ++i) {
    crc = static_cast<uint8_t>(crc >> 8) | static_cast<uint16_t>(crc << 8);
    crc ^= data[i];
    crc ^= static_cast<uint8_t>(crc & 0xff) >> 4;
    crc ^= static_cast<uint16_t>(crc << 8) << 4;
    crc ^= static_cast<uint16_t>((crc & 0xff) << 4) << 1;
  }

  return crc;
}

}  // namespace wio_ota
