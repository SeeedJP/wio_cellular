#pragma once

#include <stdint.h>

namespace wio_ota_agent {

enum class VersionStoreError {
  kNone,
  kMountFailed,
  kWriteFailed,
  kReadBackFailed,
  kRecordsInvalid,
  kNotInitialized,
};

const char* versionStoreErrorString(VersionStoreError error);

// Stores the highest version that has reached application setup(). The two
// alternating records retain the other slot while one is replaced. This is
// not a guarantee against filesystem-wide corruption or explicit formatting.
// Serialize access with other InternalFS users; do not remove/format the store
// after provisioning. The caller must stop OTA if begin/record returns false.
class VersionStore {
 public:
  // Mount without formatting, then load the highest valid slot. Both absent
  // on a mounted FS means a new store. A valid peer recovers a damaged slot;
  // existing/unreadable slots with no valid peer fail closed. An unformatted
  // FS requires explicit first provisioning outside this OTA component.
  bool begin();
  // Requires successful begin(). Non-increasing versions are no-ops. A new
  // version is accepted only after write and read-back verification; errors
  // preserve the last accepted highest version and the other slot.
  bool recordCurrentVersion(uint32_t version);

  // Meaningful only after successful initialization; not a health check.
  uint32_t highestInstalledVersion() const { return highest_version_; }
  VersionStoreError lastError() const { return last_error_; }

 private:
  uint32_t highest_version_ = 0;
  uint8_t next_slot_ = 0;
  bool ready_ = false;
  VersionStoreError last_error_ = VersionStoreError::kNone;
};

}  // namespace wio_ota_agent
