/*
 * grove-accelerometer.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>
#include <Wire.h>

#define INTERVAL (100)

#define I2C_ADDRESS (0x53)
#define REG_POWER_CTL (0x2d)
#define REG_DATAX0 (0x32)

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

  AccelInitialize();
}

void loop(void) {
  float x;
  float y;
  float z;
  AccelReadXYZ(&x, &y, &z);

  Serial.print(x);
  Serial.print(' ');
  Serial.print(y);
  Serial.print(' ');
  Serial.println(z);

  delay(INTERVAL);
}

void AccelInitialize(void) {
  Wire.begin();
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(REG_POWER_CTL);
  Wire.write(0x08);
  Wire.endTransmission();
}

void AccelReadXYZ(float* x, float* y, float* z) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(REG_DATAX0);
  Wire.endTransmission();

  if (Wire.requestFrom(I2C_ADDRESS, 6) != 6) {
    *x = 0;
    *y = 0;
    *z = 0;
    return;
  }

  int16_t val;
  ((uint8_t*)&val)[0] = Wire.read();
  ((uint8_t*)&val)[1] = Wire.read();
  *x = (float)val * 2.f / 512;
  ((uint8_t*)&val)[0] = Wire.read();
  ((uint8_t*)&val)[1] = Wire.read();
  *y = (float)val * 2.f / 512;
  ((uint8_t*)&val)[0] = Wire.read();
  ((uint8_t*)&val)[1] = Wire.read();
  *z = (float)val * 2.f / 512;
}
