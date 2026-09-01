#include "WioOtaSha256.h"

#include <cstring>

namespace wio_ota {
namespace {

constexpr uint32_t kRoundConstants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
    0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
    0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
    0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
    0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
    0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
    0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
    0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

uint32_t rotateRight(uint32_t value, unsigned int count) {
  return (value >> count) | (value << (32 - count));
}

uint32_t readBigEndian(const uint8_t* input) {
  return (static_cast<uint32_t>(input[0]) << 24) |
         (static_cast<uint32_t>(input[1]) << 16) |
         (static_cast<uint32_t>(input[2]) << 8) |
         static_cast<uint32_t>(input[3]);
}

void writeBigEndian(uint8_t* output, uint32_t value) {
  output[0] = static_cast<uint8_t>(value >> 24);
  output[1] = static_cast<uint8_t>(value >> 16);
  output[2] = static_cast<uint8_t>(value >> 8);
  output[3] = static_cast<uint8_t>(value);
}

}  // namespace

Sha256::Sha256() { reset(); }

void Sha256::reset() {
  state_[0] = 0x6a09e667;
  state_[1] = 0xbb67ae85;
  state_[2] = 0x3c6ef372;
  state_[3] = 0xa54ff53a;
  state_[4] = 0x510e527f;
  state_[5] = 0x9b05688c;
  state_[6] = 0x1f83d9ab;
  state_[7] = 0x5be0cd19;
  total_size_ = 0;
  buffer_size_ = 0;
  std::memset(buffer_, 0, sizeof(buffer_));
}

void Sha256::transform(const uint8_t block[64]) {
  uint32_t words[64];
  for (size_t i = 0; i < 16; ++i) {
    words[i] = readBigEndian(block + i * 4);
  }
  for (size_t i = 16; i < 64; ++i) {
    const uint32_t s0 = rotateRight(words[i - 15], 7) ^
                        rotateRight(words[i - 15], 18) ^
                        (words[i - 15] >> 3);
    const uint32_t s1 = rotateRight(words[i - 2], 17) ^
                        rotateRight(words[i - 2], 19) ^
                        (words[i - 2] >> 10);
    words[i] = words[i - 16] + s0 + words[i - 7] + s1;
  }

  uint32_t a = state_[0];
  uint32_t b = state_[1];
  uint32_t c = state_[2];
  uint32_t d = state_[3];
  uint32_t e = state_[4];
  uint32_t f = state_[5];
  uint32_t g = state_[6];
  uint32_t h = state_[7];

  for (size_t i = 0; i < 64; ++i) {
    const uint32_t sum1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^
                          rotateRight(e, 25);
    const uint32_t choice = (e & f) ^ (~e & g);
    const uint32_t temp1 =
        h + sum1 + choice + kRoundConstants[i] + words[i];
    const uint32_t sum0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^
                          rotateRight(a, 22);
    const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
    const uint32_t temp2 = sum0 + majority;

    h = g;
    g = f;
    f = e;
    e = d + temp1;
    d = c;
    c = b;
    b = a;
    a = temp1 + temp2;
  }

  state_[0] += a;
  state_[1] += b;
  state_[2] += c;
  state_[3] += d;
  state_[4] += e;
  state_[5] += f;
  state_[6] += g;
  state_[7] += h;
}

void Sha256::update(const uint8_t* data, size_t size) {
  total_size_ += size;
  while (size > 0) {
    const size_t available = sizeof(buffer_) - buffer_size_;
    const size_t copy_size = size < available ? size : available;
    std::memcpy(buffer_ + buffer_size_, data, copy_size);
    buffer_size_ += copy_size;
    data += copy_size;
    size -= copy_size;

    if (buffer_size_ == sizeof(buffer_)) {
      transform(buffer_);
      buffer_size_ = 0;
    }
  }
}

void Sha256::finish(uint8_t output[kSha256Size]) {
  const uint64_t bit_length = total_size_ * 8;
  buffer_[buffer_size_++] = 0x80;

  if (buffer_size_ > 56) {
    std::memset(buffer_ + buffer_size_, 0, sizeof(buffer_) - buffer_size_);
    transform(buffer_);
    buffer_size_ = 0;
  }
  std::memset(buffer_ + buffer_size_, 0, 56 - buffer_size_);
  for (size_t i = 0; i < 8; ++i) {
    buffer_[63 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
  }
  transform(buffer_);

  for (size_t i = 0; i < 8; ++i) {
    writeBigEndian(output + i * 4, state_[i]);
  }
}

}  // namespace wio_ota
