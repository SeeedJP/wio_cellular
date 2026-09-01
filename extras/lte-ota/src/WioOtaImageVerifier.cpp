#include "WioOtaImageVerifier.h"

#include <cstring>

#include "WioOtaCrc16.h"

namespace wio_ota {

const char* verificationErrorString(VerificationError error) {
  switch (error) {
    case VerificationError::kNone:
      return "none";
    case VerificationError::kInvalidState:
      return "invalid state";
    case VerificationError::kInvalidArgument:
      return "invalid argument";
    case VerificationError::kImageIncomplete:
      return "image incomplete";
    case VerificationError::kCrcMismatch:
      return "CRC16 mismatch";
    case VerificationError::kSha256Mismatch:
      return "SHA-256 mismatch";
    case VerificationError::kCrcDisabledValue:
      return "CRC16 is zero and would disable bootloader CRC checking";
  }
  return "unknown";
}

ImageVerifier::ImageVerifier()
    : state_(State::kIdle),
      image_size_(0),
      bytes_received_(0),
      crc_(kCrc16InitialValue),
      sha_(),
      sha256_{} {}

VerificationError ImageVerifier::fail(VerificationError error) {
  state_ = State::kFailed;
  return error;
}

VerificationError ImageVerifier::begin(size_t image_size) {
  if (state_ != State::kIdle && state_ != State::kFailed) {
    return VerificationError::kInvalidState;
  }
  if (image_size == 0) {
    return fail(VerificationError::kInvalidArgument);
  }
  image_size_ = image_size;
  bytes_received_ = 0;
  crc_ = kCrc16InitialValue;
  sha_.reset();
  std::memset(sha256_, 0, sizeof(sha256_));
  state_ = State::kReceiving;
  return VerificationError::kNone;
}

VerificationError ImageVerifier::update(const uint8_t* data, size_t size) {
  if (state_ != State::kReceiving) {
    return VerificationError::kInvalidState;
  }
  if (data == nullptr || size == 0 ||
      size > image_size_ - bytes_received_) {
    return fail(VerificationError::kInvalidArgument);
  }
  crc_ = crc16Ccitt(data, size, crc_);
  sha_.update(data, size);
  bytes_received_ += size;
  return VerificationError::kNone;
}

VerificationError ImageVerifier::finish(
    uint16_t expected_crc,
    const uint8_t expected_sha256[kSha256Size]) {
  if (state_ != State::kReceiving) {
    return VerificationError::kInvalidState;
  }
  if (bytes_received_ != image_size_) {
    return fail(VerificationError::kImageIncomplete);
  }
  if (crc_ != expected_crc) {
    return fail(VerificationError::kCrcMismatch);
  }
  if (crc_ == 0) {
    return fail(VerificationError::kCrcDisabledValue);
  }
  if (expected_sha256 == nullptr) {
    return fail(VerificationError::kInvalidArgument);
  }
  sha_.finish(sha256_);
  if (std::memcmp(sha256_, expected_sha256, sizeof(sha256_)) != 0) {
    return fail(VerificationError::kSha256Mismatch);
  }
  state_ = State::kVerified;
  return VerificationError::kNone;
}

void ImageVerifier::reset() {
  state_ = State::kIdle;
  image_size_ = 0;
  bytes_received_ = 0;
  crc_ = kCrc16InitialValue;
  sha_.reset();
  std::memset(sha256_, 0, sizeof(sha256_));
}

}  // namespace wio_ota
