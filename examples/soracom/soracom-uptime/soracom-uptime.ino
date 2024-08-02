/*
 * soracom-uptime.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   ArduinoJson 7.0.4 - https://github.com/bblanchon/ArduinoJson

#include <Adafruit_TinyUSB.h>
#include <algorithm>
#include <WioCellular.h>
#include <ArduinoJson.h>

static const char APN[] = "soracom.io";
static const char HOST[] = "uni.soracom.io";
static constexpr int PORT = 23080;

static constexpr int INTERVAL = 1000 * 60 * 15;  // [ms]
static constexpr int POWER_ON_TIMEOUT = 20000;   // [ms]
static constexpr int RECEIVE_TIMEOUT = 10000;    // [ms]

#define ABORT_IF_FAILED(result) \
  do { \
    if ((result) != WioCellularResult::Ok) abort(); \
  } while (0)

static constexpr int PDP_CONTEXT_ID = 1;
static constexpr int SOCKET_ID = 0;

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
  digitalWrite(LED_BUILTIN, HIGH);

  WioCellular.begin();
  ABORT_IF_FAILED(WioCellular.powerOn(POWER_ON_TIMEOUT));

  setupCellular();

  digitalWrite(LED_BUILTIN, LOW);
}

void loop(void) {
  digitalWrite(LED_BUILTIN, HIGH);

  JsonDoc.clear();
  if (measure(JsonDoc)) {
    std::string jsonStr;
    serializeJson(JsonDoc, jsonStr);

    send(reinterpret_cast<const uint8_t*>(jsonStr.data()), jsonStr.size());
  }

  digitalWrite(LED_BUILTIN, LOW);
  delay(INTERVAL);
}

static void setupCellular(void) {
  Serial.println("### Setup cellular");

  std::vector<WioCellularModule::PdpContext> pdpContexts;
  ABORT_IF_FAILED(WioCellular.getPdpContext(&pdpContexts));

  if (std::find_if(pdpContexts.begin(), pdpContexts.end(), [](const WioCellularModule::PdpContext& pdpContext) {
        return pdpContext.apn == APN;
      })
      == pdpContexts.end()) {
    ABORT_IF_FAILED(WioCellular.setPhoneFunctionality(0));
    ABORT_IF_FAILED(WioCellular.setPdpContext({ PDP_CONTEXT_ID, "IP", APN, "0.0.0.0", 0, 0, 0 }));
    ABORT_IF_FAILED(WioCellular.setPhoneFunctionality(1));
  }

  Serial.println("### Completed");
}

static bool measure(JsonDocument& doc) {
  Serial.println("### Measuring");

  doc["uptime"] = millis() / 1000;

  Serial.println("### Completed");

  return true;
}

static bool send(const void* data, size_t size) {
  bool result = true;

  Serial.println("### Sending");

  if (result) {
    Serial.print("Connecting ");
    Serial.print(HOST);
    Serial.print(":");
    Serial.println(PORT);
    if (WioCellular.openSocket(PDP_CONTEXT_ID, SOCKET_ID, "TCP", HOST, PORT, 0) != WioCellularResult::Ok) {
      Serial.println("ERROR: Failed to open socket");
      result = false;
    }
  }

  if (result) {
    Serial.print("Sending ");
    printData(Serial, data, size);
    Serial.println();
    if (WioCellular.sendSocket(SOCKET_ID, data, size) != WioCellularResult::Ok) {
      Serial.println("ERROR: Failed to send socket");
      result = false;
    }
  }

  static uint8_t recvData[1500];
  size_t recvSize;
  if (result) {
    Serial.println("Receiving");
    if (WioCellular.receiveSocket(SOCKET_ID, recvData, sizeof(recvData), &recvSize, RECEIVE_TIMEOUT) != WioCellularResult::Ok) {
      Serial.println("ERROR: Failed to receive socket");
      result = false;
    } else {
      printData(Serial, recvData, recvSize);
      Serial.println();
    }
  }

  if (WioCellular.closeSocket(SOCKET_ID) != WioCellularResult::Ok) {
    Serial.println("ERROR: Failed to close socket");
    result = false;
  }

  if (result)
    Serial.println("### Completed");

  return result;
}

template<typename T>
void printData(T& stream, const void* data, size_t size) {
  auto p = static_cast<const char*>(data);

  for (; size > 0; --size, ++p)
    stream.write(0x20 <= *p && *p <= 0x7f ? *p : '.');
}
