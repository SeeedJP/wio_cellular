/*
 * soracom-uptime-wionetwork.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   http://librarymanager#ArduinoJson 7.0.4

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>
#include <ArduinoJson.h>

static const char APN[] = "soracom.io";
static const char HOST[] = "uni.soracom.io";
static constexpr int PORT = 23080;

static constexpr int INTERVAL = 1000 * 60 * 15;  // [ms]
static constexpr int POWER_ON_TIMEOUT = 20000;   // [ms]
static constexpr int RECEIVE_TIMEOUT = 10000;    // [ms]

static JsonDocument JsonDoc;

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

  Serial.println("Startup");

  WioCellular.begin();
  if (WioCellular.powerOn(POWER_ON_TIMEOUT) != WioCellularResult::Ok) abort();

  WioNetwork.config.apn = APN;
  WioNetwork.config.searchAccessTechnology = WioNetwork.SearchAccessTechnology::LTEM;
  WioNetwork.config.ltemBand = WioNetwork.NTTDOCOMO_LTEM_BAND;
  // WioNetwork.config.ltemBand = WioNetwork.KDDI_LTEM_BAND;
  WioNetwork.begin();
}

void loop(void) {
  if (WioNetwork.canCommunicate()) {
    digitalWrite(LED_BUILTIN, HIGH);

    JsonDoc.clear();
    if (measure(JsonDoc)) {
      send(JsonDoc);
    }

    digitalWrite(LED_BUILTIN, LOW);
  }

  Serial.flush();
  const auto start = millis();
  while (millis() - start < INTERVAL) {
    WioCellular.doWork(INTERVAL - (millis() - start));
  }
}

static bool measure(JsonDocument &doc) {
  Serial.println("### Measuring");

  doc["uptime"] = millis() / 1000;

  Serial.println("### Completed");

  return true;
}

static bool send(const JsonDocument &doc) {
  Serial.println("### Sending");

  int socketId;
  if (WioCellular.getSocketUnusedConnectId(WioNetwork.config.pdpContextId, &socketId) != WioCellularResult::Ok) {
    Serial.println("ERROR: Failed to get unused connect id");
    return false;
  }

  Serial.print("Connecting ");
  Serial.print(HOST);
  Serial.print(":");
  Serial.println(PORT);
  if (WioCellular.openSocket(WioNetwork.config.pdpContextId, socketId, "TCP", HOST, PORT, 0) != WioCellularResult::Ok) {
    Serial.println("ERROR: Failed to open socket");
    return false;
  }
  bool result = true;

  if (result) {
    Serial.print("Sending ");
    std::string str;
    serializeJson(doc, str);
    printData(Serial, str.data(), str.size());
    Serial.println();
    if (WioCellular.sendSocket(socketId, str.data(), str.size()) != WioCellularResult::Ok) {
      Serial.println("ERROR: Failed to send socket");
      result = false;
    }
  }

  static uint8_t recvData[1500];
  size_t recvSize;
  if (result) {
    Serial.println("Receiving");
    if (WioCellular.receiveSocket(socketId, recvData, sizeof(recvData), &recvSize, RECEIVE_TIMEOUT) != WioCellularResult::Ok) {
      Serial.println("ERROR: Failed to receive socket");
      result = false;
    } else {
      printData(Serial, recvData, recvSize);
      Serial.println();
    }
  }

  if (WioCellular.closeSocket(socketId) != WioCellularResult::Ok) {
    Serial.println("ERROR: Failed to close socket");
    result = false;
  }

  if (result)
    Serial.println("### Completed");

  return result;
}

template<typename T>
void printData(T &stream, const void *data, size_t size) {
  auto p = static_cast<const char *>(data);

  for (; size > 0; --size, ++p)
    stream.write(0x20 <= *p && *p <= 0x7f ? *p : '.');
}
