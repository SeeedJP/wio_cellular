#pragma once

#include <Arduino.h>
#include <WioCellular.h>
#include <WioBg770aHttp.h>
#include <WioOtaManifest.h>
#include <WioOtaSecurity.h>
#include <WioOta.h>

#include <functional>

namespace wio_ota_agent {

// Immutable connection and security settings copied into Agent. All referenced
// strings and key bytes must remain valid for the Agent's lifetime. Hosts omit
// scheme/path; paths start with '/'; ports and PDP context ID must be nonzero.
struct Config {
  const char* target_hardware = nullptr;
  const char* manifest_host = nullptr;
  uint16_t manifest_port = 80;
  const char* manifest_path = nullptr;
  const char* allowed_firmware_host = nullptr;
  uint16_t allowed_firmware_port = 80;
  int pdp_context_id = 1;
  SecurityPolicy security;
};

// Application policy returned only after manifest parsing and security checks.
enum class Decision {
  kReject,
  kNoUpdate,
  kDefer,
  kDownloadAndVerify,
  kApply,
};

// Outcome of one synchronous check. kVerified stages no bootloader update;
// kActivated normally resets on device; kFailed is explained by last*Error().
enum class Result {
  kNoUpdate,
  kDeferred,
  kRejected,
  kVerified,
  kActivated,
  kFailed,
};

// Agent-level failure category. Transport, writer and security details remain
// available from the matching last*Error() accessor.
enum class Error {
  kNone,
  kInvalidConfiguration,
  kHttpConfigurationFailed,
  kManifestUrlTooLong,
  kManifestGetFailed,
  kManifestResponseRejected,
  kManifestReadFailed,
  kManifestJsonInvalid,
  kManifestFieldsInvalid,
  kFirmwareUrlInvalid,
  kFirmwareHostRejected,
  kManifestSecurityRejected,
  kFirmwareGetFailed,
  kFirmwareResponseRejected,
  kFirmwareReadFailed,
  kModemPowerOffFailed,
  kWriterFailed,
};

const char* errorString(Error error);
const char* resultString(Result result);

class Agent {
 public:
  using DecisionCallback = std::function<Decision(const Manifest&)>;
  using ProgressCallback = std::function<void(size_t received, size_t total)>;

  // Copies Config, but not its referenced strings/key data. Keep those and
  // module/logger alive for this Agent. Store the large Agent off task stacks.
  Agent(WioCellularModule& module, const Config& config,
        Stream* logger = nullptr);
  Agent(const Agent&) = delete;
  Agent& operator=(const Agent&) = delete;

  // Synchronous, non-reentrant; caller owns LTE/PDP/PSM and exclusive modem
  // access. Security checks precede decide. Verify-only discards RAM state
  // without registering an update and can be followed by another check().
  // Apply registers Bank 1 and resets (does not return on the device). Other
  // results return to the app, which restores its normal modem/PSM policy.
  // Errors/lastManifest describe the most recent check, reset at its start.
  Result check(const DecisionCallback& decide,
               const ProgressCallback& on_progress = ProgressCallback{});

  Error lastError() const { return last_error_; }
  wio_bg770a_http::Error lastHttpError() const { return last_http_error_; }
  wio_bg770a_http::Error lastHttpCleanupError() const {
    return last_http_cleanup_error_;
  }
  // True when a failed HTTP operation forced the Agent to power off the modem
  // before returning. The application must restore LTE/PDP before retrying.
  bool lastHttpRecoveryAttempted() const {
    return last_http_recovery_attempted_;
  }
  // True when that required modem power-off command also failed.
  bool lastHttpRecoveryFailed() const { return last_http_recovery_failed_; }
  wio_ota::Error lastWriterError() const { return last_writer_error_; }
  SecurityError lastSecurityError() const { return last_security_error_; }
  const Manifest& lastManifest() const { return last_manifest_; }

 private:
  using HttpClient = wio_bg770a_http::Client<WioCellularModule>;

  bool configurationIsValid() const;
  bool fetchManifest(HttpClient& client, char* output, size_t capacity);
  bool downloadFirmware(HttpClient& client, const Manifest& manifest,
                        const ProgressCallback& on_progress);
  bool buildHttpUrl(const char* host, uint16_t port, const char* path,
                    char* output, size_t capacity) const;
  void log(const char* message) const;
  void logf(const char* format, ...) const;
  Result fail(Error error);
  Result failHttp(Error error, const HttpClient& client);
  Result failWriter(wio_ota::Error error);
  bool recoverHttpIfRequired(const HttpClient& client);

  WioCellularModule& module_;
  Config config_;
  Stream* logger_;
  wio_ota::Writer writer_;
  Manifest last_manifest_;
  Error last_error_;
  wio_bg770a_http::Error last_http_error_;
  wio_bg770a_http::Error last_http_cleanup_error_;
  bool last_http_recovery_attempted_;
  bool last_http_recovery_failed_;
  wio_ota::Error last_writer_error_;
  SecurityError last_security_error_;
};

}  // namespace wio_ota_agent
