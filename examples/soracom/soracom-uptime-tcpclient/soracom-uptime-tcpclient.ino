/*
 * soracom-uptime-tcpclient.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   http://librarymanager#ArduinoJson 7.0.4

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>
#include <ArduinoJson.h>

#define SEARCH_ACCESS_TECHNOLOGY (WioCellularNetwork::SearchAccessTechnology::LTEM)
#define LTEM_BAND (WioCellularNetwork::NTTDOCOMO_LTEM_BAND)
static const char APN[] = "soracom.io";

static const char HOST[] = "uni.soracom.io";
static constexpr int PORT = 23080;

static constexpr int INTERVAL = 1000 * 60 * 5;      // [ms]
static constexpr int POWER_ON_TIMEOUT = 1000 * 20;  // [ms]
static constexpr int RECEIVE_TIMEOUT = 1000 * 10;   // [ms]

#define ABORT_IF_FAILED(result) \
  do { \
    if ((result) != WioCellularResult::Ok) abort(); \
  } while (0)

static constexpr int PDP_CONTEXT_ID = 1;
static constexpr int SOCKET_ID = 0;

static JsonDocument JsonDoc;
static WioCellularTcpClient<WioCellularModule> TcpClient{ WioCellular, PDP_CONTEXT_ID, SOCKET_ID };

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

  WioNetwork.config.searchAccessTechnology = SEARCH_ACCESS_TECHNOLOGY;
  WioNetwork.config.ltemBand = LTEM_BAND;
  WioNetwork.config.apn = APN;
  WioNetwork.begin();

  digitalWrite(LED_BUILTIN, LOW);
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
  WioCellular.doWorkUntil(INTERVAL);
}

static bool measure(JsonDocument& doc) {
  Serial.println("### Measuring");

  doc["uptime"] = millis() / 1000;

  Serial.println("### Completed");

  return true;
}

static bool send(const JsonDocument& doc) {
  Serial.println("### Sending");

  Serial.print("Connecting ");
  Serial.print(HOST);
  Serial.print(":");
  Serial.println(PORT);
  if (!TcpClient.connect(HOST, PORT)) {
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
    if (TcpClient.write(reinterpret_cast<const uint8_t*>(str.data()), str.size()) != str.size()) {
      Serial.println("ERROR: Failed to send socket");
      result = false;
    }
  }

  if (result) {
    Serial.println("Receiving");
    int availableSize;
    const auto start = millis();
    while ((availableSize = TcpClient.available()) == 0 && millis() - start < RECEIVE_TIMEOUT) {
      delay(2);
    }
    if (availableSize <= 0) {
      Serial.println("ERROR: Failed to available socket");
      result = false;
    }
  }

  static uint8_t recvData[WioCellular.RECEIVE_SOCKET_SIZE_MAX];
  int recvSize;
  if (result) {
    recvSize = TcpClient.read(recvData, sizeof(recvData));
    if (recvSize <= 0) {
      Serial.println("ERROR: Failed to receive socket");
      result = false;
    } else {
      printData(Serial, recvData, recvSize);
      Serial.println();
    }
  }

  TcpClient.stop();

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
