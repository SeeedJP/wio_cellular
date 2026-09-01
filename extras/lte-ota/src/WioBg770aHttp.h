#pragma once

#include <Arduino.h>
#include <WioCellular.h>

#include <functional>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace wio_bg770a_http {

// Parsed +QHTTPGET response. modem_result_code is the BG770A operation code;
// status_code and content_length are the HTTP response metadata.
struct Response {
  int modem_result_code = -1;
  int status_code = -1;
  int content_length = -1;
};

// Failure reported by the most recent Client operation. Cleanup failures are
// also available separately through lastCleanupError().
enum class Error {
  kNone,
  kInvalidArgument,
  kConfigurationFailed,
  kCommandEchoTimeout,
  kPromptTimeout,
  kFinalResultTimeout,
  kCommandRejected,
  kGetResultTimeout,
  kGetFailed,
  kInvalidContentLength,
  kBodyTimeout,
  kBodySinkRejected,
  kReadResultTimeout,
  kReadFailed,
  kFileStoreTimeout,
  kFileStoreFailed,
  kFileOpenFailed,
  kFileReadFailed,
  kFileCloseFailed,
  kFileCleanupFailed,
  kModemResetRequired,
};

inline const char* errorString(Error error) {
  switch (error) {
    case Error::kNone:
      return "none";
    case Error::kInvalidArgument:
      return "invalid argument";
    case Error::kConfigurationFailed:
      return "configuration failed";
    case Error::kCommandEchoTimeout:
      return "command echo timeout";
    case Error::kPromptTimeout:
      return "data prompt timeout";
    case Error::kFinalResultTimeout:
      return "final result timeout";
    case Error::kCommandRejected:
      return "command rejected";
    case Error::kGetResultTimeout:
      return "GET result timeout";
    case Error::kGetFailed:
      return "GET failed";
    case Error::kInvalidContentLength:
      return "invalid content length";
    case Error::kBodyTimeout:
      return "body timeout";
    case Error::kBodySinkRejected:
      return "body sink rejected";
    case Error::kReadResultTimeout:
      return "READ result timeout";
    case Error::kReadFailed:
      return "READ failed";
    case Error::kFileStoreTimeout:
      return "file store timeout";
    case Error::kFileStoreFailed:
      return "file store failed";
    case Error::kFileOpenFailed:
      return "file open failed";
    case Error::kFileReadFailed:
      return "file read failed";
    case Error::kFileCloseFailed:
      return "file close failed";
    case Error::kFileCleanupFailed:
      return "file cleanup failed";
    case Error::kModemResetRequired:
      return "modem reset required";
  }
  return "unknown";
}

template <typename Module>
class Client {
 public:
  // Called for each received body block. Returning false aborts the transfer.
  using BodySink = std::function<bool(const uint8_t*, size_t)>;

  // The module must outlive the client and must not be used concurrently.
  explicit Client(Module& module, Stream* logger = nullptr)
      : module_{module},
        logger_{logger},
        last_error_{Error::kNone},
        last_cleanup_error_{Error::kNone},
        modem_reset_required_{false} {}

  // Configure the BG770A HTTP context. Returns false on command rejection or
  // timeout; a timeout also requires a modem reset before this Client is reused.
  bool configure(int context_id) {
    if (!beginOperation()) {
      return false;
    }
    if (!execute("AT+QHTTPCFG=\"contextid\"," +
                 std::to_string(context_id)) ||
        !execute("AT+QHTTPCFG=\"requestheader\",0") ||
        !execute("AT+QHTTPCFG=\"responseheader\",0")) {
      last_error_ = Error::kConfigurationFailed;
      return false;
    }
    last_error_ = Error::kNone;
    return true;
  }

  // Start an HTTP GET for a complete URL. The URL is copied to the modem
  // synchronously and Response is filled only from +QHTTPGET fields.
  bool beginGet(const char* url, Response* response) {
    if (!beginOperation()) {
      return false;
    }
    if (url == nullptr || response == nullptr) {
      last_error_ = Error::kInvalidArgument;
      return false;
    }
    const size_t url_length = strlen(url);
    if (url_length == 0 || url_length > 3000) {
      last_error_ = Error::kInvalidArgument;
      return false;
    }

    const std::string url_command =
        "AT+QHTTPURL=" + std::to_string(url_length) + ",60";
    if (!module_.writeAndWaitCommand(url_command, kCommandEchoTimeoutMs)) {
      requireModemReset(Error::kCommandEchoTimeout);
      return false;
    }
    const WaitOutcome url_prompt = waitForLine("CONNECT", kPromptTimeoutMs);
    if (url_prompt != WaitOutcome::kMatched) {
      if (url_prompt == WaitOutcome::kTimeout) {
        requireModemReset(Error::kPromptTimeout);
      } else {
        last_error_ = Error::kCommandRejected;
      }
      return false;
    }
    module_.writeBinary(url, url_length);
    const WaitOutcome url_result = waitForFinalOk(kPromptTimeoutMs);
    if (url_result != WaitOutcome::kMatched) {
      if (url_result == WaitOutcome::kTimeout) {
        requireModemReset(Error::kFinalResultTimeout);
      } else {
        last_error_ = Error::kCommandRejected;
      }
      return false;
    }

    if (!module_.writeAndWaitCommand("AT+QHTTPGET=80",
                                     kCommandEchoTimeoutMs)) {
      requireModemReset(Error::kCommandEchoTimeout);
      return false;
    }

    bool saw_ok = false;
    bool saw_get_result = false;
    const uint32_t started_at = millis();
    while (millis() - started_at < kOperationTimeoutMs) {
      const std::string line = module_.readResponse(kReadSliceMs);
      if (line.empty()) {
        continue;
      }
      if (line == "OK") {
        saw_ok = true;
        if (saw_get_result) {
          break;
        }
        continue;
      }
      if (isErrorLine(line)) {
        last_error_ = Error::kCommandRejected;
        return false;
      }
      if (parseGetResult(line, response)) {
        saw_get_result = true;
        if (saw_ok) {
          break;
        }
      }
    }
    if (!saw_ok || !saw_get_result) {
      requireModemReset(Error::kGetResultTimeout);
      return false;
    }
    if (response->modem_result_code != 0 || response->status_code < 100) {
      last_error_ = Error::kGetFailed;
      return false;
    }
    if (response->content_length < 0) {
      last_error_ = Error::kInvalidContentLength;
      return false;
    }
    last_error_ = Error::kNone;
    return true;
  }

  // Read exactly body_size bytes directly from the modem into sink. A short
  // body, rejected sink, missing final result or nonzero result fails.
  bool readBody(size_t body_size, const BodySink& sink) {
    if (!beginOperation()) {
      return false;
    }
    if (!sink) {
      last_error_ = Error::kInvalidArgument;
      return false;
    }
    if (body_size == 0) {
      last_error_ = Error::kInvalidContentLength;
      return false;
    }
    if (!module_.writeAndWaitCommand("AT+QHTTPREAD=80",
                                     kCommandEchoTimeoutMs)) {
      requireModemReset(Error::kCommandEchoTimeout);
      return false;
    }
    const WaitOutcome read_prompt =
        waitForLine("CONNECT", kOperationTimeoutMs);
    if (read_prompt != WaitOutcome::kMatched) {
      if (read_prompt == WaitOutcome::kTimeout) {
        requireModemReset(Error::kPromptTimeout);
      } else {
        last_error_ = Error::kCommandRejected;
      }
      return false;
    }

    static uint8_t buffer[kReadChunkSize];
    size_t received = 0;
    while (received < body_size) {
      const size_t request =
          min(body_size - received, static_cast<size_t>(sizeof(buffer)));
      if (!module_.readBinary(buffer, request, kOperationTimeoutMs)) {
        requireModemReset(Error::kBodyTimeout);
        return false;
      }
      if (!sink(buffer, request)) {
        requireModemReset(Error::kBodySinkRejected);
        return false;
      }
      received += request;
    }

    bool saw_ok = false;
    bool saw_read_result = false;
    int read_result = -1;
    const uint32_t started_at = millis();
    while (millis() - started_at < kReadResultTimeoutMs) {
      const std::string line = module_.readResponse(kReadSliceMs);
      if (line.empty()) {
        continue;
      }
      if (line == "OK") {
        saw_ok = true;
        if (saw_read_result) {
          break;
        }
        continue;
      }
      if (isErrorLine(line)) {
        last_error_ = Error::kReadFailed;
        return false;
      }
      if (sscanf(line.c_str(), "+QHTTPREAD: %d", &read_result) == 1) {
        saw_read_result = true;
        if (saw_ok) {
          break;
        }
      }
    }
    if (!saw_ok || !saw_read_result) {
      requireModemReset(Error::kReadResultTimeout);
      return false;
    }
    if (read_result != 0) {
      last_error_ = Error::kReadFailed;
      return false;
    }
    last_error_ = Error::kNone;
    return true;
  }

  // Store the HTTP body in the modem scratch file, stream exactly body_size
  // bytes to sink, then close and delete the scratch file. On failure,
  // lastError() preserves the transfer error while lastCleanupError() reports
  // any close/delete recovery failure. This call is synchronous.
  bool readBodyViaFile(size_t body_size, const BodySink& sink) {
    if (!beginOperation()) {
      return false;
    }
    if (!sink) {
      last_error_ = Error::kInvalidArgument;
      return false;
    }
    if (body_size == 0) {
      last_error_ = Error::kInvalidContentLength;
      return false;
    }

    // A previous interrupted OTA may have left the scratch file behind.
    // File-not-found is harmless, but a timeout can leave a delayed response.
    const WioCellularResult stale_delete_result =
        module_.executeCommand("AT+QFDEL=\"wio_ota.bin\"", 1000);
    if (isTimeoutResult(stale_delete_result)) {
      last_cleanup_error_ = Error::kFileCleanupFailed;
      requireModemReset(Error::kFileCleanupFailed);
      return false;
    }

    if (!module_.writeAndWaitCommand(
            "AT+QHTTPREADFILE=\"wio_ota.bin\",80",
            kCommandEchoTimeoutMs)) {
      requireModemReset(Error::kCommandEchoTimeout);
      return false;
    }
    bool saw_ok = false;
    bool saw_store_result = false;
    int store_result = -1;
    const uint32_t store_started_at = millis();
    while (millis() - store_started_at < kOperationTimeoutMs) {
      const std::string line = module_.readResponse(kReadSliceMs);
      if (line.empty()) {
        continue;
      }
      if (line == "OK") {
        saw_ok = true;
        if (saw_store_result) {
          break;
        }
        continue;
      }
      if (isErrorLine(line)) {
        cleanupFile(-1);
        last_error_ = Error::kFileStoreFailed;
        return false;
      }
      if (sscanf(line.c_str(), "+QHTTPREADFILE: %d", &store_result) == 1) {
        saw_store_result = true;
        if (saw_ok) {
          break;
        }
      }
    }
    if (!saw_ok || !saw_store_result) {
      // A QHTTPREADFILE completion URC may arrive after the operation timeout.
      // Drain it for a bounded interval before trying to remove the scratch
      // file so the next check does not consume a stale response.
      const bool drained = drainStoreResult(kStoreRecoveryTimeoutMs);
      if (!drained) {
        modem_reset_required_ = true;
      } else {
        cleanupFile(-1);
      }
      last_error_ = Error::kFileStoreTimeout;
      return false;
    }
    if (store_result != 0) {
      cleanupFile(-1);
      last_error_ = Error::kFileStoreFailed;
      return false;
    }
    log("[HTTP] UFS response stored");

    int file_handle = -1;
    const WioCellularResult open_result = module_.queryCommand(
        "AT+QFOPEN=\"wio_ota.bin\",2",
        [&file_handle](const std::string& line) {
          return sscanf(line.c_str(), "+QFOPEN: %d", &file_handle) == 1;
        },
        1000);
    const bool open_timed_out = isTimeoutResult(open_result);
    if (open_timed_out) {
      modem_reset_required_ = true;
    }
    if (open_result != WioCellularResult::Ok || file_handle < 0) {
      if (!open_timed_out) {
        cleanupFile(-1);
      }
      last_error_ = Error::kFileOpenFailed;
      return false;
    }
    logf("[HTTP] UFS file opened handle=%d", file_handle);

    static uint8_t buffer[kReadChunkSize];
    size_t received = 0;
    while (received < body_size) {
      const size_t request =
          min(body_size - received, static_cast<size_t>(sizeof(buffer)));
      const std::string command = "AT+QFREAD=" + std::to_string(file_handle) +
                                  "," + std::to_string(request);
      if (received == 0) {
        logf("[HTTP] UFS first read request=%u",
             static_cast<unsigned>(request));
      }
      if (!module_.writeAndWaitCommand(command, kCommandEchoTimeoutMs)) {
        requireModemReset(Error::kFileReadFailed);
        return false;
      }
      if (received == 0) {
        log("[HTTP] UFS first read echo received");
      }

      int read_length = -1;
      bool prompt_rejected = false;
      const uint32_t prompt_started_at = millis();
      while (millis() - prompt_started_at < kPromptTimeoutMs) {
        const std::string line = module_.readResponse(kReadSliceMs);
        if (sscanf(line.c_str(), "CONNECT %d", &read_length) == 1) {
          break;
        }
        if (!line.empty() && isErrorLine(line)) {
          prompt_rejected = true;
          break;
        }
      }
      if (read_length <= 0 || static_cast<size_t>(read_length) != request) {
        if (prompt_rejected) {
          cleanupFile(file_handle);
          last_error_ = Error::kFileReadFailed;
        } else {
          requireModemReset(Error::kFileReadFailed);
        }
        return false;
      }
      if (!module_.readBinary(buffer, request, kOperationTimeoutMs)) {
        requireModemReset(Error::kFileReadFailed);
        return false;
      }
      if (received == 0) {
        logf("[HTTP] UFS first read payload=%d", read_length);
      }
      if (!sink(buffer, request)) {
        requireModemReset(Error::kFileReadFailed);
        return false;
      }
      const WaitOutcome read_result = waitForFinalOk(kPromptTimeoutMs);
      if (read_result != WaitOutcome::kMatched) {
        if (read_result == WaitOutcome::kTimeout) {
          requireModemReset(Error::kFileReadFailed);
        } else {
          cleanupFile(file_handle);
          last_error_ = Error::kFileReadFailed;
        }
        return false;
      }
      received += request;
    }

    if (!cleanupFile(file_handle)) {
      last_error_ = last_cleanup_error_ == Error::kFileCloseFailed
                        ? Error::kFileCloseFailed
                        : Error::kFileCleanupFailed;
      return false;
    }
    last_error_ = Error::kNone;
    return true;
  }

  // Primary error from the most recent operation.
  Error lastError() const { return last_error_; }
  // Close/delete error encountered while recovering from the primary error.
  Error lastCleanupError() const { return last_cleanup_error_; }
  // True when an interrupted command may leave unread bytes or a delayed URC.
  // Do not reuse this Client until the application has reset/powered off the
  // modem and called acknowledgeModemReset().
  bool modemResetRequired() const { return modem_reset_required_; }
  // Mark a response as deliberately abandoned without reading its body.
  void abandonResponse() { modem_reset_required_ = true; }
  // Clear the reuse guard only after the modem has actually been reset or
  // powered off by the owner.
  void acknowledgeModemReset() {
    modem_reset_required_ = false;
    resetErrors();
  }

 private:
  enum class WaitOutcome { kMatched, kRejected, kTimeout };

  static constexpr uint32_t kCommandEchoTimeoutMs = 60000;
  static constexpr uint32_t kPromptTimeoutMs = 60000;
  static constexpr uint32_t kOperationTimeoutMs = 90000;
  static constexpr uint32_t kStoreRecoveryTimeoutMs = 45000;
  static constexpr uint32_t kReadResultTimeoutMs = 10000;
  static constexpr uint32_t kReadSliceMs = 1000;
  static constexpr size_t kReadChunkSize = 512;

  bool execute(const std::string& command) {
    const WioCellularResult result = module_.executeCommand(command, 1000);
    if (isTimeoutResult(result)) {
      modem_reset_required_ = true;
    }
    return result == WioCellularResult::Ok;
  }

  static bool isTimeoutResult(WioCellularResult result) {
    switch (result) {
      case WioCellularResult::WaitCommandTimeout:
      case WioCellularResult::ReadResponseTimeout:
      case WioCellularResult::RdyTimeout:
      case WioCellularResult::OpenTimeout:
      case WioCellularResult::ReceiveTimeout:
      case WioCellularResult::ConnectTimeout:
        return true;
      default:
        return false;
    }
  }

  bool beginOperation() {
    resetErrors();
    if (modem_reset_required_) {
      last_error_ = Error::kModemResetRequired;
      return false;
    }
    return true;
  }

  void requireModemReset(Error error) {
    last_error_ = error;
    modem_reset_required_ = true;
  }

  void log(const char* message) const {
    if (logger_ != nullptr) {
      logger_->println(message);
    }
  }

  void logf(const char* format, ...) const {
    if (logger_ == nullptr) {
      return;
    }
    char buffer[160] = {};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logger_->println(buffer);
  }

  static bool isErrorLine(const std::string& line) {
    return line == "ERROR" || line.compare(0, 12, "+CME ERROR: ") == 0 ||
           line.compare(0, 12, "+CMS ERROR: ") == 0;
  }

  WaitOutcome waitForLine(const char* expected, uint32_t timeout_ms) {
    const uint32_t started_at = millis();
    while (millis() - started_at < timeout_ms) {
      const std::string line = module_.readResponse(kReadSliceMs);
      if (line == expected) {
        return WaitOutcome::kMatched;
      }
      if (!line.empty() && isErrorLine(line)) {
        return WaitOutcome::kRejected;
      }
    }
    return WaitOutcome::kTimeout;
  }

  WaitOutcome waitForFinalOk(uint32_t timeout_ms) {
    const uint32_t started_at = millis();
    while (millis() - started_at < timeout_ms) {
      const std::string line = module_.readResponse(kReadSliceMs);
      if (line == "OK") {
        return WaitOutcome::kMatched;
      }
      if (!line.empty() && isErrorLine(line)) {
        return WaitOutcome::kRejected;
      }
    }
    return WaitOutcome::kTimeout;
  }

  static bool parseGetResult(const std::string& line, Response* response) {
    int modem_result_code = -1;
    int status_code = -1;
    int content_length = -1;
    const int fields =
        sscanf(line.c_str(), "+QHTTPGET: %d,%d,%d", &modem_result_code,
               &status_code, &content_length);
    if (fields < 1) {
      return false;
    }
    response->modem_result_code = modem_result_code;
    response->status_code = fields >= 2 ? status_code : -1;
    response->content_length = fields >= 3 ? content_length : -1;
    return true;
  }

  void resetErrors() {
    last_error_ = Error::kNone;
    last_cleanup_error_ = Error::kNone;
  }

  bool drainStoreResult(uint32_t timeout_ms) {
    const uint32_t started_at = millis();
    while (millis() - started_at < timeout_ms) {
      const std::string line = module_.readResponse(kReadSliceMs);
      int store_result = -1;
      if (sscanf(line.c_str(), "+QHTTPREADFILE: %d", &store_result) == 1 ||
          (!line.empty() && isErrorLine(line))) {
        return true;
      }
    }
    return false;
  }

  bool cleanupFile(int file_handle) {
    WioCellularResult close_result = WioCellularResult::Ok;
    if (file_handle >= 0) {
      close_result = module_.executeCommand(
          "AT+QFCLOSE=" + std::to_string(file_handle), 1000);
    }
    if (isTimeoutResult(close_result)) {
      modem_reset_required_ = true;
      last_cleanup_error_ = Error::kFileCloseFailed;
      return false;
    }
    const WioCellularResult delete_result =
        module_.executeCommand("AT+QFDEL=\"wio_ota.bin\"", 1000);
    if (isTimeoutResult(delete_result)) {
      modem_reset_required_ = true;
    }
    const bool closed = close_result == WioCellularResult::Ok;
    const bool deleted = delete_result == WioCellularResult::Ok;
    if (!closed) {
      last_cleanup_error_ = Error::kFileCloseFailed;
    } else if (!deleted) {
      last_cleanup_error_ = Error::kFileCleanupFailed;
    }
    return closed && deleted;
  }

  Module& module_;
  Stream* logger_;
  Error last_error_;
  Error last_cleanup_error_;
  bool modem_reset_required_;
};

}  // namespace wio_bg770a_http
