/*
 * flash.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   Adafruit SPIFlash 4.3.4 - https://github.com/adafruit/Adafruit_SPIFlash
//   SdFat - Adafruit Fork 2.2.3 - https://github.com/adafruit/SdFat

#include <Adafruit_TinyUSB.h>
#include <Adafruit_SPIFlash.h>

static const SPIFlash_Device_t SPIFLASH_DEVICE = FERAM_DEVICE_CONFIG;

static SPIClass FlashSpi(FERAM_SPI, PIN_FERAM_SO, PIN_FERAM_SCK, PIN_FERAM_SI);
static Adafruit_FlashTransport_SPI FlashTransport(PIN_FERAM_CS, FlashSpi);
static Adafruit_SPIFlash Flash(&FlashTransport);

void setup(void) {
  Serial.begin(115200);
  {
    const auto start = millis();
    while (!Serial && millis() - start < 5000) {
      delay(2);
    }
  }
  Serial.println();
  Serial.println();

  Serial.println("Initialize");
  digitalWrite(PIN_FERAM_WP, HIGH);
  digitalWrite(PIN_FERAM_HOLD, HIGH);
  pinMode(PIN_FERAM_WP, OUTPUT);
  pinMode(PIN_FERAM_HOLD, OUTPUT);
  Flash.begin(&SPIFLASH_DEVICE, 1);

  Serial.print("JEDEC ID:   0x");
  Serial.println(Flash.getJEDECID(), HEX);

  if (digitalRead(PIN_BUTTON1) == LOW) {
    Serial.println("Reset");
    writeValue(0);

    while (digitalRead(PIN_BUTTON1) == LOW) delay(2);
    delay(100);
  }
}

void loop(void) {
  auto value = readValue();
  Serial.println(value);

  delay(500);
  while (digitalRead(PIN_BUTTON1) == HIGH) delay(2);

  ++value;
  writeValue(value);
}

uint32_t readValue(void) {
  uint32_t value;

  if (Flash.readBuffer(0, reinterpret_cast<uint8_t*>(&value), sizeof(value)) != sizeof(value)) abort();

  return value;
}

void writeValue(uint32_t value) {
  if (Flash.writeBuffer(0, reinterpret_cast<uint8_t*>(&value), sizeof(value)) != sizeof(value)) abort();
}
