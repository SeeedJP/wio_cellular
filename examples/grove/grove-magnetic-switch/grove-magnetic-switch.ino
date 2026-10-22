/*
 * grove-magnetic-switch.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

#define MAGNETIC_SWITCH_PIN (D30)  // Grove - Digital (P1)
#define INTERVAL (100)

void setup() {
  Serial.begin(115200);
  {
    const auto start = millis();
    while (!Serial && millis() - start < 5000) {
      delay(2);
    }
  }
  Serial.println();
  Serial.println();

  WioCellular.begin();
  digitalWrite(PIN_VGROVE_ENABLE, VGROVE_ENABLE_ON);
  delay(2 + 2);

  pinMode(MAGNETIC_SWITCH_PIN, INPUT);
}

void loop() {
  static int count = 0;

  const auto magneticSwitchState = digitalRead(MAGNETIC_SWITCH_PIN);
  Serial.print(magneticSwitchState ? '*' : '.');

  if (++count >= 10) {
    Serial.println();
    count = 0;
  }

  delay(INTERVAL);
}
