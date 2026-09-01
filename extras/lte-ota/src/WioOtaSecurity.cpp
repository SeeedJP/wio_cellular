#include "WioOtaSecurity.h"
#include "WioOtaBuildConfig.h"

#include <cstring>

#include <WioOtaSha256.h>

#if WIO_OTA_HAS_ED25519
#include <Adafruit_nRFCrypto.h>
#include <nrf_cc310/include/crys_ec_edw_api.h>
#endif

namespace wio_ota_agent {
namespace {

#if defined(WIO_OTA_NATIVE_TEST)
NativeSignatureVerifier native_signature_verifier = nullptr;
#endif

bool nonEmpty(const char* value) {
  return value != nullptr && value[0] != '\0';
}

SecurityError verifyEd25519(const Manifest& manifest,
                            const uint8_t* public_key) {
#if WIO_OTA_HAS_ED25519
  uint8_t canonical[kManifestCanonicalCapacity] = {};
  size_t canonical_size = 0;
  if (!encodeManifestCanonical(manifest, canonical, sizeof(canonical),
                               &canonical_size)) {
    return SecurityError::kCanonicalEncodingFailed;
  }
  if (!nRFCrypto.begin()) {
    return SecurityError::kVerifierUnavailable;
  }
  auto* temporary = static_cast<CRYS_ECEDW_TempBuff_t*>(
      rtos_malloc(sizeof(CRYS_ECEDW_TempBuff_t)));
  if (temporary == nullptr) {
    return SecurityError::kVerifierUnavailable;
  }
  const CRYSError_t result = CRYS_ECEDW_Verify(
      manifest.signature, sizeof(manifest.signature), public_key,
      kManifestEd25519PublicKeySize, canonical, canonical_size, temporary);
  rtos_free(temporary);
  return result == CRYS_OK ? SecurityError::kNone
                           : SecurityError::kSignatureInvalid;
#else
#if defined(WIO_OTA_NATIVE_TEST)
  if (native_signature_verifier != nullptr) {
    return native_signature_verifier(manifest, public_key);
  }
#else
  (void)manifest;
  (void)public_key;
#endif
  return SecurityError::kVerifierUnavailable;
#endif
}

}  // namespace

const char* securityErrorString(SecurityError error) {
  switch (error) {
    case SecurityError::kNone:
      return "none";
    case SecurityError::kInvalidPolicy:
      return "invalid security policy";
    case SecurityError::kSignatureRequired:
      return "signed manifest required";
    case SecurityError::kKeyIdRejected:
      return "manifest key ID rejected";
    case SecurityError::kCanonicalEncodingFailed:
      return "manifest canonical encoding failed";
    case SecurityError::kVerifierUnavailable:
      return "Ed25519 verifier unavailable";
    case SecurityError::kSignatureInvalid:
      return "manifest signature invalid";
    case SecurityError::kAlreadyInstalled:
      return "manifest version already installed";
    case SecurityError::kRollbackRejected:
      return "manifest version rejected by anti-rollback policy";
    case SecurityError::kRolloutRequired:
      return "format-2 rollout metadata required";
    case SecurityError::kRolloutNotSelected:
      return "device not selected for rollout";
  }
  return "unknown";
}

bool manifestRolloutBucket(const char* device_id, const char* release_id,
                           uint16_t* bucket) {
  if (!nonEmpty(device_id) || !nonEmpty(release_id) || bucket == nullptr) {
    return false;
  }
  wio_ota::Sha256 sha256;
  sha256.update(reinterpret_cast<const uint8_t*>(device_id),
                std::strlen(device_id));
  const uint8_t separator = 0;
  sha256.update(&separator, sizeof(separator));
  sha256.update(reinterpret_cast<const uint8_t*>(release_id),
                std::strlen(release_id));
  uint8_t digest[wio_ota::kSha256Size] = {};
  sha256.finish(digest);
  const uint32_t prefix = (static_cast<uint32_t>(digest[0]) << 24) |
                          (static_cast<uint32_t>(digest[1]) << 16) |
                          (static_cast<uint32_t>(digest[2]) << 8) |
                          static_cast<uint32_t>(digest[3]);
  *bucket = static_cast<uint16_t>(prefix % 10000u);
  return true;
}

SecurityError evaluateManifestSecurity(const Manifest& manifest,
                                       const SecurityPolicy& policy) {
  if (policy.require_signature) {
    if (policy.manifest_public_key == nullptr ||
        !nonEmpty(policy.expected_key_id)) {
      return SecurityError::kInvalidPolicy;
    }
    if (manifest.format != 2 || !manifest.has_signature) {
      return SecurityError::kSignatureRequired;
    }
    if (std::strcmp(manifest.key_id, policy.expected_key_id) != 0) {
      return SecurityError::kKeyIdRejected;
    }
    const SecurityError signature_error =
        verifyEd25519(manifest, policy.manifest_public_key);
    if (signature_error != SecurityError::kNone) {
      return signature_error;
    }
  }

  if (policy.enforce_anti_rollback) {
    const uint32_t floor =
        policy.current_version > policy.highest_installed_version
            ? policy.current_version
            : policy.highest_installed_version;
    if (manifest.version < floor) {
      return SecurityError::kRollbackRejected;
    }
    if (manifest.version == floor) {
      return SecurityError::kAlreadyInstalled;
    }
  }

  if (policy.enforce_rollout && manifest.format != 2) {
    return SecurityError::kRolloutRequired;
  }
  if (policy.enforce_rollout && manifest.rollout_basis_points < 10000) {
    uint16_t bucket = 0;
    if (!manifestRolloutBucket(policy.rollout_device_id, manifest.release_id,
                               &bucket)) {
      return SecurityError::kInvalidPolicy;
    }
    if (bucket >= manifest.rollout_basis_points) {
      return SecurityError::kRolloutNotSelected;
    }
  }
  return SecurityError::kNone;
}

#if defined(WIO_OTA_NATIVE_TEST)
void setNativeSignatureVerifierForTest(NativeSignatureVerifier verifier) {
  native_signature_verifier = verifier;
}
#endif

}  // namespace wio_ota_agent
