/*
 * inspection.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   http://librarymanager#AceButton 1.10.1

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>
#include <AceButton.h>

using namespace ace_button;

static constexpr int POWER_ON_TIMEOUT = 1000 * 20;  // [ms]

static const char* CELLULAR_REVISION = "BG770AGLAAR02A05_JP_01.200.01.200";

static AceButton UserButton(PIN_BUTTON1);
static bool UserButtonPressed = false;
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
  auto config = UserButton.getButtonConfig();
  config->setEventHandler(userButtonHandler);
  config->setDebounceDelay(50);
  WioCellular.begin();

  Serial.println("Wait for USER button");
  UserButtonPressed = false;
  while (true) {
    if (millis() % 400 < 200)
      ledOn(LED_BUILTIN);
    else
      ledOff(LED_BUILTIN);

    UserButton.check();
    if (UserButtonPressed) break;

    delay(2);
  }
  ledOn(LED_BUILTIN);
  Serial.println();

  Serial.print("Enable Grove ... ");
  WioCellular.enableGrovePower();
  Serial.println("OK");

  Serial.println("Wait for USER button");
  UserButtonPressed = false;
  while (true) {
    UserButton.check();
    if (UserButtonPressed) break;
    delay(2);
  }

  Serial.print("Disable Grove ... ");
  WioCellular.disableGrovePower();
  Serial.println("OK");

  Serial.println("Wait for USER button");
  UserButtonPressed = false;
  while (true) {
    UserButton.check();
    if (UserButtonPressed) break;
    delay(2);
  }

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

static void userButtonHandler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventPressed:
      UserButtonPressed = true;
      break;
  }
}
