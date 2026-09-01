#include "WioOtaManifest.h"

#include <ArduinoJson.h>

#include <cstring>
#include <limits>

namespace wio_ota_agent {
namespace {

struct FirmwareLocation {
  char host[128] = {};
  char path[256] = {};
  uint16_t port = 80;
};

bool copyString(const char* source, char* destination, size_t capacity) {
  if (source == nullptr || destination == nullptr || capacity == 0) {
    return false;
  }
  const size_t length = std::strlen(source);
  if (length >= capacity) {
    return false;
  }
  std::memcpy(destination, source, length + 1);
  return true;
}

int hexNibble(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  return -1;
}

bool decodeHex(const char* hex, uint8_t* output, size_t output_size) {
  if (hex == nullptr || output == nullptr ||
      std::strlen(hex) != output_size * 2) {
    return false;
  }
  for (size_t i = 0; i < output_size; ++i) {
    const int high = hexNibble(hex[i * 2]);
    const int low = hexNibble(hex[i * 2 + 1]);
    if (high < 0 || low < 0) {
      return false;
    }
    output[i] = static_cast<uint8_t>((high << 4) | low);
  }
  return true;
}

bool isSafeText(const char* text, bool allow_empty) {
  if (text == nullptr || (!allow_empty && text[0] == '\0')) {
    return false;
  }
  for (const unsigned char* cursor =
           reinterpret_cast<const unsigned char*>(text);
       *cursor != '\0'; ++cursor) {
    if (*cursor < 0x20 || *cursor == 0x7f) {
      return false;
    }
  }
  return true;
}

bool appendBytes(const uint8_t* data, size_t size, uint8_t* output,
                 size_t capacity, size_t* position) {
  if (data == nullptr || output == nullptr || position == nullptr ||
      *position > capacity || size > capacity - *position) {
    return false;
  }
  std::memcpy(output + *position, data, size);
  *position += size;
  return true;
}

bool appendU16(uint16_t value, uint8_t* output, size_t capacity,
               size_t* position) {
  const uint8_t encoded[] = {static_cast<uint8_t>(value >> 8),
                             static_cast<uint8_t>(value)};
  return appendBytes(encoded, sizeof(encoded), output, capacity, position);
}

bool appendU32(uint32_t value, uint8_t* output, size_t capacity,
               size_t* position) {
  const uint8_t encoded[] = {
      static_cast<uint8_t>(value >> 24), static_cast<uint8_t>(value >> 16),
      static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
  return appendBytes(encoded, sizeof(encoded), output, capacity, position);
}

bool appendString(const char* value, uint8_t* output, size_t capacity,
                  size_t* position) {
  if (value == nullptr) {
    return false;
  }
  const size_t length = std::strlen(value);
  if (length > std::numeric_limits<uint16_t>::max() ||
      !appendU16(static_cast<uint16_t>(length), output, capacity, position)) {
    return false;
  }
  return appendBytes(reinterpret_cast<const uint8_t*>(value), length, output,
                     capacity, position);
}

bool parseHttpUrl(const char* url, FirmwareLocation* location) {
  constexpr char kPrefix[] = "http://";
  if (url == nullptr || location == nullptr ||
      std::strncmp(url, kPrefix, sizeof(kPrefix) - 1) != 0) {
    return false;
  }
  const char* authority = url + sizeof(kPrefix) - 1;
  const char* path = std::strchr(authority, '/');
  if (path == nullptr ||
      !copyString(path, location->path, sizeof(location->path))) {
    return false;
  }
  const char* colon = static_cast<const char*>(
      std::memchr(authority, ':', static_cast<size_t>(path - authority)));
  const char* host_end = colon == nullptr ? path : colon;
  const size_t host_length = static_cast<size_t>(host_end - authority);
  if (host_length == 0 || host_length >= sizeof(location->host)) {
    return false;
  }
  std::memcpy(location->host, authority, host_length);
  location->host[host_length] = '\0';

  if (colon != nullptr) {
    uint32_t parsed_port = 0;
    if (colon + 1 == path) {
      return false;
    }
    for (const char* cursor = colon + 1; cursor != path; ++cursor) {
      if (*cursor < '0' || *cursor > '9') {
        return false;
      }
      parsed_port = parsed_port * 10u + static_cast<uint32_t>(*cursor - '0');
      if (parsed_port > std::numeric_limits<uint16_t>::max()) {
        return false;
      }
    }
    if (parsed_port == 0) {
      return false;
    }
    location->port = static_cast<uint16_t>(parsed_port);
  }
  return true;
}

}  // namespace

const char* manifestErrorString(ManifestError error) {
  switch (error) {
    case ManifestError::kNone:
      return "none";
    case ManifestError::kInvalidPolicy:
      return "invalid policy";
    case ManifestError::kJsonInvalid:
      return "manifest JSON invalid";
    case ManifestError::kFieldsInvalid:
      return "manifest fields invalid";
    case ManifestError::kFirmwareUrlInvalid:
      return "firmware URL invalid";
    case ManifestError::kFirmwareHostRejected:
      return "firmware host rejected";
  }
  return "unknown";
}

ManifestError parseManifest(const char* json, const ManifestPolicy& policy,
                            Manifest* manifest) {
  if (json == nullptr || manifest == nullptr) {
    return ManifestError::kJsonInvalid;
  }
  if (policy.target_hardware == nullptr ||
      policy.target_hardware[0] == '\0' ||
      policy.allowed_firmware_host == nullptr ||
      policy.allowed_firmware_host[0] == '\0' ||
      policy.allowed_firmware_port == 0 || policy.maximum_image_size < 8) {
    return ManifestError::kInvalidPolicy;
  }

  JsonDocument document;
  const DeserializationError json_error = deserializeJson(document, json);
  if (json_error) {
    return ManifestError::kJsonInvalid;
  }

  const int format = document["format"] | -1;
  const char* hardware = document["hardware"];
  const int64_t version = document["version"] | int64_t{-1};
  const int64_t image_size = document["size"] | int64_t{-1};
  const char* crc_text = document["crc16"];
  const char* sha_text = document["sha256"];
  const char* url = document["url"];
  if ((format != 1 && format != 2) || hardware == nullptr ||
      std::strcmp(hardware, policy.target_hardware) != 0 || version < 0 ||
      static_cast<uint64_t>(version) >
          std::numeric_limits<uint32_t>::max() ||
      image_size < 8 ||
      static_cast<uint64_t>(image_size) > policy.maximum_image_size) {
    return ManifestError::kFieldsInvalid;
  }

  Manifest parsed;
  if (!copyString(hardware, parsed.hardware, sizeof(parsed.hardware)) ||
      !copyString(url, parsed.url, sizeof(parsed.url)) ||
      !isSafeText(parsed.hardware, false) || !isSafeText(parsed.url, false)) {
    return ManifestError::kFieldsInvalid;
  }

  uint8_t crc_bytes[2] = {};
  if (!decodeHex(crc_text, crc_bytes, sizeof(crc_bytes)) ||
      !decodeHex(sha_text, parsed.sha256, sizeof(parsed.sha256))) {
    return ManifestError::kFieldsInvalid;
  }
  parsed.crc16 =
      (static_cast<uint16_t>(crc_bytes[0]) << 8) | crc_bytes[1];
  if (parsed.crc16 == 0) {
    return ManifestError::kFieldsInvalid;
  }

  if (format == 2) {
    const char* release_id = document["release_id"];
    const int64_t rollout = document["rollout"] | int64_t{-1};
    const char* key_id = document["key_id"];
    const char* signature = document["signature"];
    if (rollout < 0 || rollout > 10000 ||
        !copyString(release_id, parsed.release_id,
                    sizeof(parsed.release_id)) ||
        !copyString(key_id, parsed.key_id, sizeof(parsed.key_id)) ||
        !isSafeText(parsed.release_id, false) ||
        !isSafeText(parsed.key_id, false) ||
        !decodeHex(signature, parsed.signature, sizeof(parsed.signature))) {
      return ManifestError::kFieldsInvalid;
    }
    parsed.rollout_basis_points = static_cast<uint16_t>(rollout);
    parsed.has_signature = true;
  }

  FirmwareLocation location;
  if (!parseHttpUrl(parsed.url, &location)) {
    return ManifestError::kFirmwareUrlInvalid;
  }
  if (std::strcmp(location.host, policy.allowed_firmware_host) != 0 ||
      location.port != policy.allowed_firmware_port) {
    return ManifestError::kFirmwareHostRejected;
  }

  parsed.format = format;
  parsed.version = static_cast<uint32_t>(version);
  parsed.image_size = static_cast<size_t>(image_size);
  parsed.firmware_port = location.port;
  copyString(location.host, parsed.firmware_host,
             sizeof(parsed.firmware_host));
  copyString(location.path, parsed.firmware_path,
             sizeof(parsed.firmware_path));
  *manifest = parsed;
  return ManifestError::kNone;
}

bool encodeManifestCanonical(const Manifest& manifest, uint8_t* output,
                             size_t capacity, size_t* output_size) {
  constexpr uint8_t kMagic[] = "WIO-OTA-MANIFEST-V2";
  if (output == nullptr || output_size == nullptr || manifest.format != 2 ||
      !manifest.has_signature || !isSafeText(manifest.hardware, false) ||
      !isSafeText(manifest.url, false) ||
      !isSafeText(manifest.release_id, false) ||
      !isSafeText(manifest.key_id, false) ||
      manifest.image_size > std::numeric_limits<uint32_t>::max() ||
      manifest.rollout_basis_points > 10000) {
    return false;
  }

  size_t position = 0;
  if (!appendBytes(kMagic, sizeof(kMagic) - 1, output, capacity, &position) ||
      !appendU32(static_cast<uint32_t>(manifest.format), output, capacity,
                 &position) ||
      !appendString(manifest.hardware, output, capacity, &position) ||
      !appendU32(manifest.version, output, capacity, &position) ||
      !appendString(manifest.url, output, capacity, &position) ||
      !appendU32(static_cast<uint32_t>(manifest.image_size), output, capacity,
                 &position) ||
      !appendU16(manifest.crc16, output, capacity, &position) ||
      !appendBytes(manifest.sha256, sizeof(manifest.sha256), output, capacity,
                   &position) ||
      !appendString(manifest.release_id, output, capacity, &position) ||
      !appendU16(manifest.rollout_basis_points, output, capacity, &position) ||
      !appendString(manifest.key_id, output, capacity, &position)) {
    return false;
  }
  *output_size = position;
  return true;
}

}  // namespace wio_ota_agent
