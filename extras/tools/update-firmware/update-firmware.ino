/*
 * update-firmware.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

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
  WioCellular.begin();

  Serial.println("Press USER button to turn on cellular");
  while (true) {
    if (millis() % 400 < 200)
      ledOn(LED_BUILTIN);
    else
      ledOff(LED_BUILTIN);

    if (digitalRead(PIN_BUTTON1) == LOW) break;

    delay(2);
  }

  Serial.println("Turn on cellular");
  ledOn(LED_BUILTIN);
  WioCellular.getInterface().powerOn();

  Serial.println("Ready");
}

void loop(void) {
  int c;

  while (true) {
    while ((c = Serial.read()) >= 0) {
      WioCellular.getInterface().write(c);
    }

    while ((c = WioCellular.getInterface().read()) >= 0) {
      Serial.write(c);
    }

    delay(2);
  }
}
