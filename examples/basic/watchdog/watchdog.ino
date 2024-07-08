/*
 * watchdog.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   Adafruit SleepyDog Library 1.6.5 - https://github.com/adafruit/Adafruit_SleepyDog

#include <Adafruit_TinyUSB.h>
#include <Adafruit_SleepyDog.h>

static uint32_t start;

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

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Enable watchdog");
  Watchdog.enable(10000);
  start = millis();
}

void loop(void) {
  Serial.println(millis() - start);

  if (digitalRead(PIN_BUTTON1) == LOW) {
    Serial.println("Reset watchdog");
    Watchdog.reset();
  }

  delay(1000);
}
