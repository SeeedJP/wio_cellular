#pragma once

#include <stddef.h>
#include <stdint.h>

namespace wio_ota {

constexpr size_t kSha256Size = 32;

// Incremental SHA-256 calculator. Construction and reset() start a new digest.
// update() accepts ordered blocks (data may be null only for size zero).
// finish() writes 32 bytes and finalizes the current digest; call reset()
// before hashing another message.
class Sha256 {
 public:
  Sha256();

  // Discard current state and start an empty digest.
  void reset();
  // Append one input block.
  void update(const uint8_t* data, size_t size);
  // Finalize into a writable 32-byte output buffer.
  void finish(uint8_t output[kSha256Size]);

 private:
  void transform(const uint8_t block[64]);

  uint32_t state_[8];
  uint64_t total_size_;
  uint8_t buffer_[64];
  size_t buffer_size_;
};

}  // namespace wio_ota
