#include "WioOta.h"

#include <Arduino.h>
#include <cstring>
#include <nrfx_nvmc.h>
#include <nrf_sdm.h>

namespace wio_ota {
namespace {

#if defined(WIO_OTA_NATIVE_TEST) && (defined(ARDUINO) || defined(NRF52840_XXAA))
#error "Host flash emulation must never be enabled in device firmware"
#endif

// All memory-mapped reads share this boundary. Native tests use RAM while
// compiling the same writer state machine; device builds always read flash.
const void* flashAddress(uint32_t address) {
#if defined(WIO_OTA_NATIVE_TEST)
  return wioOtaTestFlashAddress(address);
#else
  return reinterpret_cast<const void*>(address);
#endif
}

constexpr uint16_t kBankValidApp = 0x0001;
constexpr uint16_t kSettingsVersion = 1;
constexpr uint32_t kRamStart = 0x20000000;
constexpr uint32_t kRamEnd = 0x20040000;

struct __attribute__((packed)) BootloaderSettings {
  uint16_t bank_0;
  uint16_t bank_0_crc;
  uint16_t bank_1;
  uint16_t dummy;
  uint32_t bank_0_size;
  uint32_t sd_image_size;
  uint32_t bl_image_size;
  uint32_t app_image_size;
  uint32_t sd_image_start;
  uint16_t settings_version;
  uint16_t bank_1_crc;
  uint32_t bank_1_size;
};

static_assert(sizeof(BootloaderSettings) == 36,
              "Bootloader settings layout must match SeeedJP bootloader");
static_assert(kBank1Address + kMaximumImageSize <= kBootloaderAddress,
              "Bank 1 must not overlap the bootloader");

alignas(4) uint32_t settings_page[kFlashPageSize / sizeof(uint32_t)];

bool isWordAligned(uint32_t value) { return (value & 0x3u) == 0; }

Error mapVerificationError(VerificationError error) {
  switch (error) {
    case VerificationError::kNone:
      return Error::kNone;
    case VerificationError::kInvalidState:
      return Error::kInvalidState;
    case VerificationError::kInvalidArgument:
      return Error::kInvalidArgument;
    case VerificationError::kImageIncomplete:
      return Error::kImageIncomplete;
    case VerificationError::kCrcMismatch:
      return Error::kCrcMismatch;
    case VerificationError::kSha256Mismatch:
      return Error::kSha256Mismatch;
    case VerificationError::kCrcDisabledValue:
      return Error::kCrcDisabledValue;
  }
  return Error::kInvalidState;
}

}  // namespace

const char* errorString(Error error) {
  switch (error) {
    case Error::kNone:
      return "none";
    case Error::kInvalidState:
      return "invalid state";
    case Error::kInvalidArgument:
      return "invalid argument";
    case Error::kImageTooLarge:
      return "image too large";
    case Error::kSoftDeviceEnabled:
      return "SoftDevice is enabled";
    case Error::kFlashEraseFailed:
      return "flash erase failed";
    case Error::kFlashWriteFailed:
      return "flash write failed";
    case Error::kFlashVerifyFailed:
      return "flash verification failed";
    case Error::kImageIncomplete:
      return "image incomplete";
    case Error::kCrcMismatch:
      return "CRC16 mismatch";
    case Error::kSha256Mismatch:
      return "SHA-256 mismatch";
    case Error::kCrcDisabledValue:
      return "CRC16 is zero and would disable bootloader CRC checking";
    case Error::kInvalidVectorTable:
      return "invalid application vector table";
    case Error::kIncompatibleBootloaderSettings:
      return "incompatible bootloader settings";
  }
  return "unknown";
}

Writer::Writer() : state_(State::kIdle), verifier_() {}

Error Writer::fail(Error error) {
  state_ = State::kFailed;
  return error;
}

Error Writer::requireSoftDeviceDisabled() const {
  uint8_t enabled = 0;
  const uint32_t result = sd_softdevice_is_enabled(&enabled);
  if (result != NRF_SUCCESS || enabled != 0) {
    return Error::kSoftDeviceEnabled;
  }
  return Error::kNone;
}

Error Writer::eraseImagePages(size_t image_size) {
  const size_t erase_size =
      (image_size + kFlashPageSize - 1) & ~(kFlashPageSize - 1);
  for (size_t offset = 0; offset < erase_size; offset += kFlashPageSize) {
    if (nrfx_nvmc_page_erase(kBank1Address + offset) != NRFX_SUCCESS) {
      return Error::kFlashEraseFailed;
    }
  }
  return Error::kNone;
}

Error Writer::begin(size_t image_size) {
  if (state_ != State::kIdle && state_ != State::kFailed) {
    return Error::kInvalidState;
  }
  // A previous hardware or vector-table failure can leave the pure verifier
  // in Receiving or Verified state. A new transfer always starts cleanly.
  verifier_.reset();
  if (image_size < 8) {
    return fail(Error::kInvalidArgument);
  }
  if (image_size > kMaximumImageSize) {
    return fail(Error::kImageTooLarge);
  }
  if (const Error error = requireSoftDeviceDisabled(); error != Error::kNone) {
    return fail(error);
  }
  if (const Error error = eraseImagePages(image_size); error != Error::kNone) {
    return fail(error);
  }

  const VerificationError verification_error = verifier_.begin(image_size);
  if (verification_error != VerificationError::kNone) {
    return fail(mapVerificationError(verification_error));
  }
  state_ = State::kReceiving;
  return Error::kNone;
}

Error Writer::writeAndVerify(uint32_t address, const void* data, size_t size) {
  nrfx_nvmc_bytes_write(address, data, size);
  if (std::memcmp(flashAddress(address), data, size) != 0) {
    return Error::kFlashVerifyFailed;
  }
  return Error::kNone;
}

Error Writer::write(const uint8_t* data, size_t size) {
  if (state_ != State::kReceiving) {
    return Error::kInvalidState;
  }
  if (data == nullptr || size == 0 ||
      size > verifier_.imageSize() - verifier_.bytesReceived()) {
    return fail(Error::kInvalidArgument);
  }
  if (const Error error = requireSoftDeviceDisabled(); error != Error::kNone) {
    return fail(error);
  }

  const uint32_t address = kBank1Address + verifier_.bytesReceived();
  if (const Error error = writeAndVerify(address, data, size);
      error != Error::kNone) {
    return fail(error);
  }

  const VerificationError verification_error = verifier_.update(data, size);
  if (verification_error != VerificationError::kNone) {
    return fail(mapVerificationError(verification_error));
  }
  return Error::kNone;
}

Error Writer::validateVectorTable() const {
  const auto* vectors = static_cast<const uint32_t*>(flashAddress(kBank1Address));
  const uint32_t initial_stack_pointer = vectors[0];
  const uint32_t reset_handler = vectors[1];
  const uint32_t reset_address = reset_handler & ~1u;

  const bool stack_valid = isWordAligned(initial_stack_pointer) &&
                           initial_stack_pointer >= kRamStart &&
                           initial_stack_pointer <= kRamEnd;
  const bool reset_valid = (reset_handler & 1u) != 0 &&
                           reset_address >= kApplicationAddress &&
                           reset_address <
                               kApplicationAddress + verifier_.imageSize();
  return stack_valid && reset_valid ? Error::kNone
                                    : Error::kInvalidVectorTable;
}

Error Writer::finish(uint16_t expected_crc,
                     const uint8_t expected_sha256[kSha256Size]) {
  if (state_ != State::kReceiving) {
    return Error::kInvalidState;
  }
  const VerificationError verification_error =
      verifier_.finish(expected_crc, expected_sha256);
  if (verification_error != VerificationError::kNone) {
    return fail(mapVerificationError(verification_error));
  }
  if (const Error error = validateVectorTable(); error != Error::kNone) {
    return fail(error);
  }

  state_ = State::kVerified;
  return Error::kNone;
}

Error Writer::activate() {
  if (state_ != State::kVerified) {
    return Error::kInvalidState;
  }
  if (const Error error = requireSoftDeviceDisabled(); error != Error::kNone) {
    return fail(error);
  }

  std::memcpy(settings_page,
              flashAddress(kBootloaderSettingsAddress),
              kFlashPageSize);
  auto* settings = reinterpret_cast<BootloaderSettings*>(settings_page);
  if (settings->settings_version != kSettingsVersion ||
      settings->bank_0 != kBankValidApp) {
    return fail(Error::kIncompatibleBootloaderSettings);
  }

  settings->bank_1 = kBankValidApp;
  settings->bank_1_crc = verifier_.calculatedCrc();
  settings->bank_1_size = verifier_.imageSize();

  if (nrfx_nvmc_page_erase(kBootloaderSettingsAddress) != NRFX_SUCCESS) {
    return fail(Error::kFlashEraseFailed);
  }
  if (const Error error =
          writeAndVerify(kBootloaderSettingsAddress, settings_page,
                         kFlashPageSize);
      error != Error::kNone) {
    return fail(error);
  }

  state_ = State::kActivated;
  return Error::kNone;
}

void Writer::discard() {
  state_ = State::kIdle;
  verifier_.reset();
}

void Writer::resetToApply() const {
  if (state_ == State::kActivated) {
    NVIC_SystemReset();
  }
}

}  // namespace wio_ota
