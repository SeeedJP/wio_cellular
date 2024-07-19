/*
 * grove-button.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

#define BUTTON_PIN (D30)  // Grove - Digital (P1)
#define INTERVAL (100)

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

  pinMode(BUTTON_PIN, INPUT);
}

void loop(void) {
  static int count = 0;

  int buttonState = digitalRead(BUTTON_PIN);
  Serial.print(buttonState ? '*' : '.');

  if (++count >= 10) {
    Serial.println();
    count = 0;
  }

  delay(INTERVAL);
}
