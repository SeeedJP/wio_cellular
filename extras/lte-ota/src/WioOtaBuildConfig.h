#pragma once

// Arduino compiles libraries separately: a #define in a sketch is not visible
// here. Make the verifier available by default in Arduino IDE / CLI nRF52840
// builds. The runtime require_signature policy still decides whether to use it.
// Preserve PlatformIO's explicit opt-in and the portable native-test fallback.
#if defined(NRF52840_XXAA) &&                                          \
    (defined(WIO_OTA_ENABLE_ED25519) || defined(WIO_OTA_SECURE) ||       \
     (defined(ARDUINO) && !defined(PLATFORMIO)))
#define WIO_OTA_HAS_ED25519 1
#else
#define WIO_OTA_HAS_ED25519 0
#endif
