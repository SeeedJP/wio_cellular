/*
 * transparent.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

static constexpr int POWER_ON_TIMEOUT = 20000;   // [ms]

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

  Serial.println("Turn on cellular");
  if (WioCellular.powerOn(POWER_ON_TIMEOUT) != WioCellularResult::Ok) {
    Serial.println("ERROR");
    abort();
  }

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
