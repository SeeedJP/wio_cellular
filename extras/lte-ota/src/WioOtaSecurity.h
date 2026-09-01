#pragma once

#include <stddef.h>
#include <stdint.h>

#include "WioOtaManifest.h"

namespace wio_ota_agent {

// Security requirements applied after manifest parsing and before the
// application's decision callback. Referenced key and identifier storage must
// outlive the Agent using this policy.
struct SecurityPolicy {
  // Require a format-2 Ed25519 signature from expected_key_id.
  bool require_signature = false;
  const uint8_t* manifest_public_key = nullptr;
  const char* expected_key_id = nullptr;

  // Reject versions below the greater of current and highest installed;
  // equality reports already installed.
  bool enforce_anti_rollback = false;
  uint32_t current_version = 0;
  uint32_t highest_installed_version = 0;

  // Require format-2 rollout metadata and select by device/release hash.
  bool enforce_rollout = false;
  const char* rollout_device_id = nullptr;
};

// Security policy result. Manifest authenticity is checked before rollback and
// rollout; any failure prevents the application callback from running.
enum class SecurityError {
  kNone,
  kInvalidPolicy,
  kSignatureRequired,
  kKeyIdRejected,
  kCanonicalEncodingFailed,
  kVerifierUnavailable,
  kSignatureInvalid,
  kAlreadyInstalled,
  kRollbackRejected,
  kRolloutRequired,
  kRolloutNotSelected,
};

const char* securityErrorString(SecurityError error);

// Returns a stable bucket in the range [0, 9999]. The raw device identifier
// is never placed in the manifest or log; only its SHA-256-derived bucket is
// used for rollout selection.
bool manifestRolloutBucket(const char* device_id, const char* release_id,
                           uint16_t* bucket);

// Evaluate signature, rollback and rollout in that order. Legacy format-1
// manifests cannot satisfy signature or rollout enforcement.
SecurityError evaluateManifestSecurity(const Manifest& manifest,
                                       const SecurityPolicy& policy);

#if defined(WIO_OTA_NATIVE_TEST)
// Host tests inject only the Ed25519 boundary while exercising the production
// canonical encoding and policy order. This symbol is absent from device builds.
using NativeSignatureVerifier = SecurityError (*)(const Manifest&,
                                                   const uint8_t* public_key);
void setNativeSignatureVerifierForTest(NativeSignatureVerifier verifier);
#endif

}  // namespace wio_ota_agent
