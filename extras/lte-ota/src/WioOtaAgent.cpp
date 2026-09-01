#include "WioOtaAgent.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#if defined(WIO_OTA_TEST_HALT_BEFORE_ACTIVATE) && \
    defined(WIO_OTA_TEST_HALT_AFTER_ACTIVATE)
#error "Select only one OTA test activation halt point"
#endif

namespace wio_ota_agent {
constexpr size_t kManifestCapacity = 1536;
static_assert(kManifestSha256Size == wio_ota::kSha256Size,
              "manifest and writer SHA-256 sizes must match");

const char* errorString(Error error) {
  switch (error) {
    case Error::kNone:
      return "none";
    case Error::kInvalidConfiguration:
      return "invalid configuration";
    case Error::kHttpConfigurationFailed:
      return "HTTP configuration failed";
    case Error::kManifestUrlTooLong:
      return "manifest URL too long";
    case Error::kManifestGetFailed:
      return "manifest GET failed";
    case Error::kManifestResponseRejected:
      return "manifest response rejected";
    case Error::kManifestReadFailed:
      return "manifest read failed";
    case Error::kManifestJsonInvalid:
      return "manifest JSON invalid";
    case Error::kManifestFieldsInvalid:
      return "manifest fields invalid";
    case Error::kFirmwareUrlInvalid:
      return "firmware URL invalid";
    case Error::kFirmwareHostRejected:
      return "firmware host rejected";
    case Error::kManifestSecurityRejected:
      return "manifest security policy rejected update";
    case Error::kFirmwareGetFailed:
      return "firmware GET failed";
    case Error::kFirmwareResponseRejected:
      return "firmware response rejected";
    case Error::kFirmwareReadFailed:
      return "firmware read failed";
    case Error::kModemPowerOffFailed:
      return "modem power-off failed";
    case Error::kWriterFailed:
      return "Bank 1 writer failed";
  }
  return "unknown";
}

const char* resultString(Result result) {
  switch (result) {
    case Result::kNoUpdate:
      return "no update";
    case Result::kDeferred:
      return "deferred";
    case Result::kRejected:
      return "rejected";
    case Result::kVerified:
      return "verified";
    case Result::kActivated:
      return "activated";
    case Result::kFailed:
      return "failed";
  }
  return "unknown";
}

Agent::Agent(WioCellularModule& module, const Config& config, Stream* logger)
    : module_{module},
      config_{config},
      logger_{logger},
      writer_{},
      last_manifest_{},
      last_error_{Error::kNone},
      last_http_error_{wio_bg770a_http::Error::kNone},
      last_http_cleanup_error_{wio_bg770a_http::Error::kNone},
      last_http_recovery_attempted_{false},
      last_http_recovery_failed_{false},
      last_writer_error_{wio_ota::Error::kNone},
      last_security_error_{SecurityError::kNone} {}

bool Agent::configurationIsValid() const {
  return config_.target_hardware != nullptr &&
         config_.target_hardware[0] != '\0' &&
         config_.manifest_host != nullptr &&
         config_.manifest_host[0] != '\0' && config_.manifest_port != 0 &&
         config_.manifest_path != nullptr &&
         config_.manifest_path[0] == '/' &&
         config_.allowed_firmware_host != nullptr &&
         config_.allowed_firmware_host[0] != '\0' &&
         config_.allowed_firmware_port != 0 && config_.pdp_context_id > 0 &&
         (!config_.security.require_signature ||
          (config_.security.manifest_public_key != nullptr &&
           config_.security.expected_key_id != nullptr &&
           config_.security.expected_key_id[0] != '\0')) &&
         (!config_.security.enforce_rollout ||
          (config_.security.rollout_device_id != nullptr &&
           config_.security.rollout_device_id[0] != '\0'));
}

void Agent::log(const char* message) const {
  if (logger_ != nullptr) {
    logger_->println(message);
  }
}

void Agent::logf(const char* format, ...) const {
  if (logger_ == nullptr) {
    return;
  }
  char buffer[256] = {};
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  logger_->println(buffer);
}

Result Agent::fail(Error error) {
  last_error_ = error;
  logf("[OTA] failed: %s", errorString(error));
  return Result::kFailed;
}

Result Agent::failHttp(Error error, const HttpClient& client) {
  last_http_error_ = client.lastError();
  last_http_cleanup_error_ = client.lastCleanupError();
  if (!recoverHttpIfRequired(client)) {
    return fail(Error::kModemPowerOffFailed);
  }
  last_error_ = error;
  logf("[HTTP] failed: %s (%s)", errorString(error),
       wio_bg770a_http::errorString(last_http_error_));
  if (last_http_cleanup_error_ != wio_bg770a_http::Error::kNone) {
    logf("[HTTP] cleanup also failed: %s",
         wio_bg770a_http::errorString(last_http_cleanup_error_));
  }
  return Result::kFailed;
}

bool Agent::recoverHttpIfRequired(const HttpClient& client) {
  if (!client.modemResetRequired()) {
    return true;
  }
  last_http_recovery_attempted_ = true;
  log("[HTTP] interrupted modem command; powering modem off before return");
  if (module_.powerOff() != WioCellularResult::Ok) {
    last_http_recovery_failed_ = true;
    log("[HTTP] required modem power-off failed");
    return false;
  }
  return true;
}

Result Agent::failWriter(wio_ota::Error error) {
  last_writer_error_ = error;
  last_error_ = Error::kWriterFailed;
  logf("[OTA] writer failed: %s", wio_ota::errorString(error));
  return Result::kFailed;
}

bool Agent::buildHttpUrl(const char* host, uint16_t port, const char* path,
                         char* output, size_t capacity) const {
  if (host == nullptr || path == nullptr || output == nullptr || capacity == 0) {
    return false;
  }
  const int length =
      port == 80 ? snprintf(output, capacity, "http://%s%s", host, path)
                 : snprintf(output, capacity, "http://%s:%u%s", host, port,
                            path);
  return length > 0 && static_cast<size_t>(length) < capacity;
}

bool Agent::fetchManifest(HttpClient& client, char* output, size_t capacity) {
  char url[512] = {};
  if (!buildHttpUrl(config_.manifest_host, config_.manifest_port,
                    config_.manifest_path, url, sizeof(url))) {
    last_error_ = Error::kManifestUrlTooLong;
    return false;
  }
  log("[HTTP] GET metadata manifest");
  wio_bg770a_http::Response response;
  if (!client.beginGet(url, &response)) {
    last_http_error_ = client.lastError();
    last_error_ = Error::kManifestGetFailed;
    return false;
  }
  logf("[HTTP] manifest status=%d length=%d", response.status_code,
       response.content_length);
  if (response.status_code != 200 || response.content_length <= 0 ||
      static_cast<size_t>(response.content_length) >= capacity) {
    client.abandonResponse();
    last_error_ = Error::kManifestResponseRejected;
    return false;
  }

  size_t received = 0;
  if (!client.readBody(
          static_cast<size_t>(response.content_length),
          [output, capacity, &received](const uint8_t* data, size_t size) {
            if (received + size >= capacity) {
              return false;
            }
            memcpy(output + received, data, size);
            received += size;
            return true;
          })) {
    last_http_error_ = client.lastError();
    last_error_ = Error::kManifestReadFailed;
    return false;
  }
  output[received] = '\0';
  return true;
}

bool Agent::downloadFirmware(HttpClient& client, const Manifest& manifest,
                             const ProgressCallback& on_progress) {
  log("[HTTP] GET firmware image");
  wio_bg770a_http::Response response;
  if (!client.beginGet(manifest.url, &response)) {
    last_http_error_ = client.lastError();
    last_error_ = Error::kFirmwareGetFailed;
    return false;
  }
  logf("[HTTP] firmware status=%d length=%d", response.status_code,
       response.content_length);
  if (response.status_code != 200 || response.content_length < 0 ||
      static_cast<size_t>(response.content_length) != manifest.image_size) {
    client.abandonResponse();
    last_error_ = Error::kFirmwareResponseRejected;
    return false;
  }

  if (const auto error = writer_.begin(manifest.image_size);
      error != wio_ota::Error::kNone) {
    client.abandonResponse();
    last_writer_error_ = error;
    last_error_ = Error::kWriterFailed;
    return false;
  }

  if (!client.readBodyViaFile(
          manifest.image_size,
          [this, &on_progress](const uint8_t* data, size_t size) {
            if (const auto error = writer_.write(data, size);
                error != wio_ota::Error::kNone) {
              last_writer_error_ = error;
              last_error_ = Error::kWriterFailed;
              return false;
            }
            if (on_progress) {
              on_progress(writer_.bytesWritten(), writer_.imageSize());
            }
            return true;
          })) {
    last_http_cleanup_error_ = client.lastCleanupError();
    if (last_error_ != Error::kWriterFailed) {
      last_http_error_ = client.lastError();
      last_error_ = Error::kFirmwareReadFailed;
    }
    writer_.discard();
    return false;
  }

  if (const auto error = writer_.finish(manifest.crc16, manifest.sha256);
      error != wio_ota::Error::kNone) {
    last_writer_error_ = error;
    last_error_ = Error::kWriterFailed;
    writer_.discard();
    return false;
  }
  log("[OTA] image verified");
  return true;
}

Result Agent::check(const DecisionCallback& decide,
                    const ProgressCallback& on_progress) {
  last_error_ = Error::kNone;
  last_http_error_ = wio_bg770a_http::Error::kNone;
  last_http_cleanup_error_ = wio_bg770a_http::Error::kNone;
  last_http_recovery_attempted_ = false;
  last_http_recovery_failed_ = false;
  last_writer_error_ = wio_ota::Error::kNone;
  last_security_error_ = SecurityError::kNone;
  last_manifest_ = Manifest{};
  if (!configurationIsValid() || !decide) {
    return fail(Error::kInvalidConfiguration);
  }

  HttpClient client{module_, logger_};
  if (!client.configure(config_.pdp_context_id)) {
    return failHttp(Error::kHttpConfigurationFailed, client);
  }
  log("[HTTP] modem HTTP client configured");

  // OTA checks are serialized. Keep the receive buffer off the Arduino task
  // stack because the application also owns the parsed Manifest object.
  static char manifest_json[kManifestCapacity] = {};
  manifest_json[0] = '\0';
  if (!fetchManifest(client, manifest_json, sizeof(manifest_json))) {
    if (last_http_error_ != wio_bg770a_http::Error::kNone) {
      return failHttp(last_error_, client);
    }
    if (!recoverHttpIfRequired(client)) {
      return fail(Error::kModemPowerOffFailed);
    }
    return fail(last_error_);
  }
  ManifestPolicy manifest_policy;
  manifest_policy.target_hardware = config_.target_hardware;
  manifest_policy.allowed_firmware_host = config_.allowed_firmware_host;
  manifest_policy.allowed_firmware_port = config_.allowed_firmware_port;
  manifest_policy.maximum_image_size = wio_ota::kMaximumImageSize;
  const ManifestError manifest_error =
      parseManifest(manifest_json, manifest_policy, &last_manifest_);
  if (manifest_error != ManifestError::kNone) {
    switch (manifest_error) {
      case ManifestError::kInvalidPolicy:
        last_error_ = Error::kInvalidConfiguration;
        break;
      case ManifestError::kJsonInvalid:
        last_error_ = Error::kManifestJsonInvalid;
        break;
      case ManifestError::kFieldsInvalid:
        last_error_ = Error::kManifestFieldsInvalid;
        break;
      case ManifestError::kFirmwareUrlInvalid:
        last_error_ = Error::kFirmwareUrlInvalid;
        break;
      case ManifestError::kFirmwareHostRejected:
        last_error_ = Error::kFirmwareHostRejected;
        break;
      case ManifestError::kNone:
        break;
    }
    return fail(last_error_);
  }

  last_security_error_ =
      evaluateManifestSecurity(last_manifest_, config_.security);
  if (last_security_error_ == SecurityError::kAlreadyInstalled) {
    logf("[OTA] no update: %s", securityErrorString(last_security_error_));
    return Result::kNoUpdate;
  }
  if (last_security_error_ == SecurityError::kRolloutNotSelected) {
    logf("[OTA] deferred: %s", securityErrorString(last_security_error_));
    return Result::kDeferred;
  }
  if (last_security_error_ != SecurityError::kNone) {
    last_error_ = Error::kManifestSecurityRejected;
    logf("[OTA] rejected: %s", securityErrorString(last_security_error_));
    return last_security_error_ == SecurityError::kInvalidPolicy ||
                   last_security_error_ ==
                       SecurityError::kVerifierUnavailable ||
                   last_security_error_ ==
                       SecurityError::kCanonicalEncodingFailed
               ? Result::kFailed
               : Result::kRejected;
  }

  const Decision decision = decide(last_manifest_);
  switch (decision) {
    case Decision::kReject:
      log("[OTA] update rejected by application");
      return Result::kRejected;
    case Decision::kNoUpdate:
      log("[OTA] application reports no update");
      return Result::kNoUpdate;
    case Decision::kDefer:
      log("[OTA] update deferred by application");
      return Result::kDeferred;
    case Decision::kDownloadAndVerify:
    case Decision::kApply:
      break;
  }

  logf("[OTA] downloading version=%lu size=%u",
       static_cast<unsigned long>(last_manifest_.version),
       static_cast<unsigned>(last_manifest_.image_size));
  if (!downloadFirmware(client, last_manifest_, on_progress)) {
    if (last_error_ == Error::kWriterFailed) {
      if (!recoverHttpIfRequired(client)) {
        return fail(Error::kModemPowerOffFailed);
      }
      return failWriter(last_writer_error_);
    }
    if (last_http_error_ != wio_bg770a_http::Error::kNone) {
      return failHttp(last_error_, client);
    }
    if (!recoverHttpIfRequired(client)) {
      return fail(Error::kModemPowerOffFailed);
    }
    return fail(last_error_);
  }

  if (decision == Decision::kDownloadAndVerify) {
    log("[OTA] verified only; application did not request activation");
    writer_.discard();
    return Result::kVerified;
  }

  if (module_.powerOff() != WioCellularResult::Ok) {
    writer_.discard();
    return fail(Error::kModemPowerOffFailed);
  }
#if defined(WIO_OTA_TEST_HALT_BEFORE_ACTIVATE)
  log("[OTA TEST] verified; halted before activate; press RESET");
  if (logger_ != nullptr) {
    logger_->flush();
  }
  for (;;) {
    delay(1000);
  }
#endif
  if (const auto error = writer_.activate();
      error != wio_ota::Error::kNone) {
    return failWriter(error);
  }
#if defined(WIO_OTA_TEST_HALT_AFTER_ACTIVATE)
  log("[OTA TEST] settings committed; halted before software reset; press RESET");
  if (logger_ != nullptr) {
    logger_->flush();
  }
  for (;;) {
    delay(1000);
  }
#endif
  log("[OTA] activated; rebooting");
  if (logger_ != nullptr) {
    logger_->flush();
  }
  delay(100);
  writer_.resetToApply();
  return Result::kActivated;
}

}  // namespace wio_ota_agent
