#pragma once

#include <stddef.h>
#include <stdint.h>

#include "WioOtaImageVerifier.h"

namespace wio_ota {

constexpr uint32_t kApplicationAddress = 0x00027000;
constexpr uint32_t kBank1Address = 0x00088000;
constexpr uint32_t kMaximumImageSize = 397312;
constexpr uint32_t kBootloaderAddress = 0x000f4000;
constexpr uint32_t kBootloaderSettingsAddress = 0x000ff000;
constexpr uint32_t kFlashPageSize = 4096;

enum class Error {
  kNone,
  kInvalidState,
  kInvalidArgument,
  kImageTooLarge,
  kSoftDeviceEnabled,
  kFlashEraseFailed,
  kFlashWriteFailed,
  kFlashVerifyFailed,
  kImageIncomplete,
  kCrcMismatch,
  kSha256Mismatch,
  kCrcDisabledValue,
  kInvalidVectorTable,
  kIncompatibleBootloaderSettings,
};

const char* errorString(Error error);

class Writer {
 public:
  Writer();

  // Start from idle/failed state; erases Bank 1 pages for image_size bytes.
  // Flash operations require SoftDevice disabled. Returns kNone on success.
  Error begin(size_t image_size);
  // Append nonempty bytes in order without exceeding image_size.
  Error write(const uint8_t* data, size_t size);
  // Require the full image, CRC16/SHA-256 match and a valid vector table.
  Error finish(uint16_t expected_crc,
               const uint8_t expected_sha256[kSha256Size]);
  // Require successful finish(); commit Bank 1 in bootloader settings.
  Error activate();
  // Reset RAM state only. Does not erase Bank 1 or cancel a committed update.
  // After verify-only, discard permits begin() for the next transfer.
  void discard();
  // Reset only if activated; normally does not return in that state.
  void resetToApply() const;

  size_t bytesWritten() const { return verifier_.bytesReceived(); }
  size_t imageSize() const { return verifier_.imageSize(); }
  uint16_t calculatedCrc() const { return verifier_.calculatedCrc(); }
  const uint8_t* calculatedSha256() const {
    return verifier_.calculatedSha256();
  }
  bool isVerified() const { return state_ == State::kVerified; }
  bool isActivated() const { return state_ == State::kActivated; }

 private:
  enum class State { kIdle, kReceiving, kVerified, kActivated, kFailed };

  Error fail(Error error);
  Error requireSoftDeviceDisabled() const;
  Error validateVectorTable() const;
  Error eraseImagePages(size_t image_size);
  Error writeAndVerify(uint32_t address, const void* data, size_t size);

  State state_;
  ImageVerifier verifier_;
};

}  // namespace wio_ota
