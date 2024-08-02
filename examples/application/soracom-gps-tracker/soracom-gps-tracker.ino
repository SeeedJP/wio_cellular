/*
 * soracom-gps-tracker.ino
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

static constexpr int INTERVAL = 1000 * 60 * 5;  // [ms]
static constexpr int POWER_ON_TIMEOUT = 20000;  // [ms]
static constexpr int RECEIVE_TIMEOUT = 10000;   // [ms]

#define ABORT_IF_FAILED(result) \
  do { \
    if ((result) != WioCellularResult::Ok) abort(); \
  } while (0)

static uint32_t MeasureTime = -INTERVAL;
static String LatestGpsData;
static String LatestRegStatus;

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
    digitalWrite(LED_BUILTIN, HIGH);

    JsonDoc.clear();
    if (measure(JsonDoc)) {
      std::string jsonStr;
      serializeJson(JsonDoc, jsonStr);

      send(reinterpret_cast<const uint8_t*>(jsonStr.data()), jsonStr.size());
    }

    digitalWrite(LED_BUILTIN, LOW);

    MeasureTime = millis();
  }

  WioCellular.doWork(10);
}

static void setupCellular(void) {
  Serial.println("### Setup cellular");

  WioCellular.registerUrcHandler([](const std::string& response) -> bool {
    if (response.compare(0, 8, "+CEREG: ") == 0) {
      LatestRegStatus = response.substr(8).c_str();
      return true;
    }
    return false;
  });
  ABORT_IF_FAILED(WioCellular.setEpsNetworkRegistrationStatusUrc(2));

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

  doc["regStatus"] = LatestRegStatus.c_str();

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
