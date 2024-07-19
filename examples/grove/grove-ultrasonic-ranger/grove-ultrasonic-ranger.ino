/*
 * grove-ultrasonic-ranger.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   Grove Ultrasonic Ranger 1.0.1 - https://github.com/Seeed-Studio/Seeed_Arduino_UltrasonicRanger

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>
#include <Ultrasonic.h>  // Grove Ultrasonic Ranger

#define ULTRASONIC_PIN (D30)  // Grove - Digital (P1)
#define INTERVAL (100)

Ultrasonic UltrasonicRanger(ULTRASONIC_PIN);

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

  WioCellular.begin();
  WioCellular.enableGrovePower();
}

void loop(void) {
  long distance;
  distance = UltrasonicRanger.MeasureInCentimeters();
  Serial.print(distance);
  Serial.println("[cm]");

  delay(INTERVAL);
}
