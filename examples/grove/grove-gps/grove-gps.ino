/*
 * grove-gps.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

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

  GpsBegin();
}

void loop(void) {
  const auto data = GpsRead();
  if (data != NULL && strncmp(data, "$GPGGA,", 7) == 0) {
    Serial.println(data);
  }
}

#define GPS_OVERFLOW_STRING "OVERFLOW"

char GpsData[100];
int GpsDataLength;

void GpsBegin(void) {
  Serial1.begin(9600);
  GpsDataLength = 0;
}

const char* GpsRead(void) {
  while (true) {
    const auto data = Serial1.read();
    if (data < 0) return NULL;
    if (data == '\r') continue;
    if (data == '\n') {
      GpsData[GpsDataLength] = '\0';
      GpsDataLength = 0;
      return GpsData;
    }

    if (GpsDataLength > (int)sizeof(GpsData) - 1) {  // Overflow
      GpsDataLength = 0;
      return GPS_OVERFLOW_STRING;
    }
    GpsData[GpsDataLength++] = data;
  }

  return NULL;
}
