/*
 * soracom-gps-tracker.ino
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

static uint32_t MeasureTime = -INTERVAL;
static String LatestGpsData;

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

  WioNetwork.config.searchAccessTechnology = SEARCH_ACCESS_TECHNOLOGY;
  WioNetwork.config.ltemBand = LTEM_BAND;
  WioNetwork.config.apn = APN;
  WioNetwork.begin();

  WioCellular.enableGrovePower();
  GpsBegin();

  digitalWrite(LED_BUILTIN, LOW);
}

void loop(void) {
  const auto data = GpsRead();
  if (data != NULL && strncmp(data, "$GPGGA,", 7) == 0) {
    LatestGpsData = data;
  }

  if (millis() - MeasureTime >= INTERVAL) {
    if (WioNetwork.canCommunicate()) {
      digitalWrite(LED_BUILTIN, HIGH);

      JsonDoc.clear();
      if (measure(JsonDoc)) {
        send(JsonDoc);
      }

      digitalWrite(LED_BUILTIN, LOW);
    }

    MeasureTime = millis();
  }

  Serial.flush();
  WioCellular.doWork(10);  // Spin
}

static bool measure(JsonDocument& doc) {
  Serial.println("### Measuring");

  doc["uptime"] = millis() / 1000;

  int index[5];
  index[0] = LatestGpsData.indexOf(',');
  index[1] = index[0] >= 0 ? LatestGpsData.indexOf(',', index[0] + 1) : -1;
  index[2] = index[1] >= 0 ? LatestGpsData.indexOf(',', index[1] + 1) : -1;
  index[3] = index[2] >= 0 ? LatestGpsData.indexOf(',', index[2] + 1) : -1;
  index[4] = index[3] >= 0 ? LatestGpsData.indexOf(',', index[3] + 1) : -1;

  if (index[4] >= 0) {
    String latDmm = LatestGpsData.substring(index[1] + 1, index[2]);
    String lonDmm = LatestGpsData.substring(index[3] + 1, index[4]);

    if (latDmm.length() >= 1 && lonDmm.length() >= 1) {
      auto DMMtoDD = [](const String& dmm) -> double {
        const double dmmDouble = atof(dmm.c_str());
        const int d = (int)dmmDouble / 100;
        return (double)d + (dmmDouble - d * 100) / 60.0;
      };

      doc["lat"] = DMMtoDD(latDmm);
      doc["lon"] = DMMtoDD(lonDmm);
    }
  }

  Serial.println("### Completed");

  return true;
}

static bool send(const JsonDocument& doc) {
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

  static uint8_t recvData[WioCellular.RECEIVE_SOCKET_SIZE_MAX];
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
void printData(T& stream, const void* data, size_t size) {
  auto p = static_cast<const char*>(data);

  for (; size > 0; --size, ++p)
    stream.write(0x20 <= *p && *p <= 0x7f ? *p : '.');
}

#define GPS_OVERFLOW_STRING "OVERFLOW"

static char GpsData[100];
static int GpsDataLength;

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
