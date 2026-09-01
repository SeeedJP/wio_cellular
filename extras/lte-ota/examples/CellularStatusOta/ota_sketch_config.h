#pragma once

// Arduino IDE: edit this value before building the next firmware.
// The manifest's --version must match this number.
#ifndef APP_VERSION
#define APP_VERSION 1
#endif

// Keep signatures enabled for normal use. Generate ota_manifest_public_key.h
// beside this file before compiling. Never place the private key here.
// 0 is for explicit legacy format-1 migration/testing only.
#ifndef WIO_OTA_ARDUINO_SECURE
#define WIO_OTA_ARDUINO_SECURE 1
#endif

#if WIO_OTA_ARDUINO_SECURE
#ifndef WIO_OTA_SECURE
#define WIO_OTA_SECURE
#endif
#endif
