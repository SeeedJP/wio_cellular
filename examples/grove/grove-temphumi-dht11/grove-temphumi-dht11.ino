/*
 * grove-temphumi-dht11.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   http://librarymanager#GroveDriverPack 1.13.0

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>
#include <GroveDriverPack.h>
#include <Module/GroveTempHumiDHT11.h>

#define DHT11_PIN (D30)          // Grove - Digital (P1)
#define STARTUP_INTERVAL (2000)  // DHT11 startup interval
#define INTERVAL (2000)

HalGpio Dht11Pin(DHT11_PIN);
GroveTempHumiDHT11 TempHumi(&Dht11Pin);

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

  TempHumi.Init();
  delay(STARTUP_INTERVAL);
}

void loop() {
  TempHumi.Read();

  Serial.print("Current humidity = ");
  Serial.print(TempHumi.Humidity);
  Serial.print("%  ");
  Serial.print("temperature = ");
  Serial.print(TempHumi.Temperature);
  Serial.println("C");

  delay(INTERVAL);
}
