#pragma once

#include <stddef.h>
#include <stdint.h>

namespace wio_ota {

constexpr uint16_t kCrc16InitialValue = 0xffff;

// CRC-16/CCITT-FALSE (polynomial 0x1021, no reflection, no final XOR).
// Pass kCrc16InitialValue for the first block and the returned value for each
// subsequent block. data may be null only when size is zero.
uint16_t crc16Ccitt(const uint8_t* data, size_t size,
                    uint16_t previous = kCrc16InitialValue);

}  // namespace wio_ota
