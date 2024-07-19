/*
 * inspection.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

static constexpr int POWER_ON_TIMEOUT = 20000;  // [ms]

static const char* CELLULAR_REVISION = "BG770AGLAAR02A05_JP_01.200.01.200";

static int ErrorCount = 0;

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

  delay(1000);
  Serial.println("Wait for USER button");

  while (true) {
    if (millis() % 400 < 200)
      ledOn(LED_BUILTIN);
    else
      ledOff(LED_BUILTIN);

    if (digitalRead(PIN_BUTTON1) == LOW) break;

    delay(2);
  }
  ledOn(LED_BUILTIN);
  Serial.println();

  Serial.print("Enable Grove ... ");
  WioCellular.enableGrovePower();
  Serial.println("OK");
  delay(5000);

  Serial.print("Disable Grove ... ");
  WioCellular.disableGrovePower();
  Serial.println("OK");
  delay(5000);

  Serial.print("Power ON cellular ... ");
  {
    const auto result = WioCellular.powerOn(POWER_ON_TIMEOUT);
    if (result != WioCellularResult::Ok) {
      ++ErrorCount;
      Serial.print("ERROR ");
      Serial.println(WioCellularResultToString(result));
      return;
    } else {
      Serial.println("OK");
    }
  }

  Serial.print("Check cellular revision ... ");
  {
    std::string revision;
    const auto result = WioCellular.getModemInfo(&revision);
    if (result != WioCellularResult::Ok) {
      ++ErrorCount;
      Serial.print("ERROR ");
      Serial.println(WioCellularResultToString(result));
    } else {
      if (revision.compare(CELLULAR_REVISION) != 0) {
        ++ErrorCount;
        Serial.println("ERROR Revision is old");
      } else {
        Serial.println("OK");
      }
    }
    Serial.print(" Current revision: ");
    Serial.println(revision.c_str());
    Serial.print(" Latest revision:  ");
    Serial.println(CELLULAR_REVISION);
  }

  Serial.print("Check SIM ... ");
  {
    std::string simState;
    const auto result = WioCellular.getSimState(&simState);
    if (result != WioCellularResult::Ok) {
      ++ErrorCount;
      Serial.print("ERROR ");
      Serial.println(WioCellularResultToString(result));
    } else {
      if (simState.compare("READY") != 0) {
        ++ErrorCount;
        Serial.println("ERROR SIM is not ready");
      } else {
        Serial.println("OK");
      }
    }
    Serial.print(" SIM state: ");
    Serial.println(simState.c_str());
  }
}

void loop(void) {
  Serial.println();
  Serial.println("Completed");
  Serial.print("Number of errors: ");
  Serial.println(ErrorCount);

  ledOff(LED_BUILTIN);
  Serial.flush();
  suspendLoop();
}
