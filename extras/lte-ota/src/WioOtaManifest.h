#pragma once

#include <stddef.h>
#include <stdint.h>

namespace wio_ota_agent {

constexpr size_t kManifestSha256Size = 32;
constexpr size_t kManifestEd25519PublicKeySize = 32;
constexpr size_t kManifestEd25519SignatureSize = 64;
constexpr size_t kManifestCanonicalCapacity = 768;

// Parsed manifest. Text capacities include the trailing NUL. image_size is in
// bytes; rollout_basis_points is 0..10000. Format 1 has no signature/rollout
// metadata; format 2 requires release_id, key_id, rollout and a 64-byte
// Ed25519 signature. firmware_* fields are derived from url after validation.
struct Manifest {
  int format = -1;
  char hardware[64] = {};
  uint32_t version = 0;
  size_t image_size = 0;
  uint16_t crc16 = 0;
  uint8_t sha256[kManifestSha256Size] = {};
  char url[512] = {};
  char release_id[64] = {};
  uint16_t rollout_basis_points = 10000;
  char key_id[32] = {};
  uint8_t signature[kManifestEd25519SignatureSize] = {};
  bool has_signature = false;
  char firmware_host[128] = {};
  char firmware_path[256] = {};
  uint16_t firmware_port = 80;
};

// Parser allow-list and image-size ceiling. Referenced strings must remain
// valid for the duration of parseManifest().
struct ManifestPolicy {
  const char* target_hardware = nullptr;
  const char* allowed_firmware_host = nullptr;
  uint16_t allowed_firmware_port = 80;
  size_t maximum_image_size = 0;
};

// Parser outcome. Output Manifest is replaced only on kNone.
enum class ManifestError {
  kNone,
  kInvalidPolicy,
  kJsonInvalid,
  kFieldsInvalid,
  kFirmwareUrlInvalid,
  kFirmwareHostRejected,
};

const char* manifestErrorString(ManifestError error);
// Parse one JSON object, validate field sizes/types, require an http:// URL with
// host <128 bytes and path <256 bytes, and enforce host/port/image policy.
ManifestError parseManifest(const char* json, const ManifestPolicy& policy,
                            Manifest* manifest);

// Serializes the signed fields of a format-2 manifest into an unambiguous
// length-prefixed binary representation. The JSON field order and whitespace
// are deliberately not part of the signature. On success output_size receives
// bytes written; output must provide capacity bytes.
bool encodeManifestCanonical(const Manifest& manifest, uint8_t* output,
                             size_t capacity, size_t* output_size);

}  // namespace wio_ota_agent
