/*
 * grove-buzzer.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

#define BUZZER_PIN (D30)  // Grove - Digital (P1)
#define BUZZER_ON_TIME (100)
#define BUZZER_OFF_TIME (3000)

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

  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop(void) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(BUZZER_ON_TIME);

  digitalWrite(BUZZER_PIN, LOW);
  delay(BUZZER_OFF_TIME);
}
