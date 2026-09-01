/*
 * Based on WioCellular's cellular-status example.
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#if !defined(PLATFORMIO)
#include "ota_sketch_config.h"
#endif

#include <Adafruit_TinyUSB.h>
#include <Arduino.h>
#include <WioCellular.h>
#include <WioOtaAgent.h>

#if defined(WIO_OTA_SECURE)
#include <WioOtaVersionStore.h>
#include <nrf.h>

#include "ota_manifest_public_key.h"
#endif

#ifndef APP_VERSION
#define APP_VERSION 1
#endif

#if !defined(BOARD_VERSION_1_0)
#error "This example targets HW v1.0. Select Board Version 1.0."
#endif

namespace {

// Explicit declarations prevent Arduino's automatic prototypes from placing
// these helpers outside the anonymous namespace.
wio_ota_agent::Decision decideUpdate(const wio_ota_agent::Manifest& manifest);
void reportProgress(size_t received, size_t total);
wio_ota_agent::Result checkOta();

constexpr uint32_t kPowerOnTimeoutMs = 20UL * 1000UL;
constexpr uint32_t kNetworkTimeoutMs = 3UL * 60UL * 1000UL;
constexpr uint32_t kStatusIntervalMs = 5UL * 1000UL;

#if defined(WIO_OTA_SECURE)
wio_ota_agent::VersionStore version_store;
uint32_t loaded_highest_version = 0;
char rollout_device_id[17] = {};
bool ota_security_ready = false;
#endif

wio_ota_agent::Decision decideUpdate(
    const wio_ota_agent::Manifest& manifest) {
  return manifest.version > APP_VERSION
             ? wio_ota_agent::Decision::kApply
             : wio_ota_agent::Decision::kNoUpdate;
}

void reportProgress(size_t received, size_t total) {
  if ((received % (16 * 1024)) == 0 || received == total) {
    Serial.printf("[OTA] progress %u/%u\n",
                  static_cast<unsigned>(received),
                  static_cast<unsigned>(total));
  }
}

wio_ota_agent::Result checkOta() {
  wio_ota_agent::Config config;
  config.target_hardware = "wio-bg770a-v1.0";
  config.manifest_host = "metadata.soracom.io";
  config.manifest_path = "/v1/userdata";
  config.allowed_firmware_host = "harvest-files.soracom.io";
  config.pdp_context_id = WioNetwork.config.pdpContextId;
#if defined(WIO_OTA_SECURE)
  config.security.require_signature = true;
  config.security.manifest_public_key = wio_ota_keys::kManifestPublicKey;
  config.security.expected_key_id = wio_ota_keys::kManifestKeyId;
  config.security.enforce_anti_rollback = true;
  config.security.current_version = APP_VERSION;
  config.security.highest_installed_version =
      version_store.highestInstalledVersion();
  config.security.enforce_rollout = true;
  config.security.rollout_device_id = rollout_device_id;
#endif

  static wio_ota_agent::Agent agent{WioCellular, config, &Serial};
  return agent.check(decideUpdate, reportProgress);
}

}  // namespace

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  const uint32_t serial_started_at = millis();
  while (!Serial && millis() - serial_started_at < 5000) {
    delay(10);
  }

  Serial.printf("cellular-status + OTA, app-version=%d\n", APP_VERSION);
#if defined(WIO_OTA_SECURE)
  snprintf(rollout_device_id, sizeof(rollout_device_id), "%08lx%08lx",
           static_cast<unsigned long>(NRF_FICR->DEVICEID[1]),
           static_cast<unsigned long>(NRF_FICR->DEVICEID[0]));
  ota_security_ready = version_store.begin();
  if (ota_security_ready) {
    // Preserve the value loaded from flash before recording this boot's version.
    loaded_highest_version = version_store.highestInstalledVersion();
    ota_security_ready = version_store.recordCurrentVersion(APP_VERSION);
  }
  if (!ota_security_ready) {
    Serial.printf("[OTA] security state unavailable: %s\n",
                  wio_ota_agent::versionStoreErrorString(
                      version_store.lastError()));
  }
#endif

  WioNetwork.config.apn = "soracom.io";
  WioCellular.begin();
  if (WioCellular.powerOn(kPowerOnTimeoutMs) != WioCellularResult::Ok) {
    Serial.println("[LTE] modem power-on failed");
    return;
  }

  WioNetwork.begin();
  if (!WioNetwork.waitUntilCommunicationAvailable(kNetworkTimeoutMs)) {
    Serial.println("[LTE] network attach failed");
    return;
  }

  Serial.println("[LTE] network ready");
#if defined(WIO_OTA_SECURE)
  if (!ota_security_ready) {
    Serial.println("[OTA] skipped because anti-rollback state is unavailable");
    return;
  }
  Serial.printf("[OTA] security state loaded=%lu current=%d highest=%lu\n",
                static_cast<unsigned long>(loaded_highest_version), APP_VERSION,
                static_cast<unsigned long>(
                    version_store.highestInstalledVersion()));
#endif
  const auto result = checkOta();
  Serial.printf("[OTA] result=%s\n", wio_ota_agent::resultString(result));
}

void loop() {
  static uint32_t last_status_at = 0;
  WioCellular.doWorkUntil(100);
  if (millis() - last_status_at >= kStatusIntervalMs) {
    last_status_at = millis();
    digitalToggle(LED_BUILTIN);
    Serial.printf("[APP] version=%d uptime=%lu\n", APP_VERSION,
                  static_cast<unsigned long>(millis() / 1000));
  }
}
