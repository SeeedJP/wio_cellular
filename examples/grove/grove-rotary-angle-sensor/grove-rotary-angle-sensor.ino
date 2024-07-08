/*
 * grove-rotary-angle-sensor.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.hpp>

#define ROTARY_ANGLE_PIN (A4)  // Grove - Analog (P1)
#define INTERVAL (500)
#define BAR_LENGTH (40)

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

  analogReadResolution(14);
}

void loop(void) {
  const auto rotaryAngleRaw = analogRead(ROTARY_ANGLE_PIN);
  const auto rotaryAngle = (float)rotaryAngleRaw / 16383 * 0.6f * 6 / 3.3f;

  int i;
  for (i = 0; i < BAR_LENGTH * rotaryAngle; i++) Serial.print("*");
  for (; i < BAR_LENGTH; i++) Serial.print(".");
  Serial.print(" ");
  Serial.print(rotaryAngle);
  Serial.print("(");
  Serial.print(rotaryAngleRaw);
  Serial.println(")");

  delay(INTERVAL);
}
