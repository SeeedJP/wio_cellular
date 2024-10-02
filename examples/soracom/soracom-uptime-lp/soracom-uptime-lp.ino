/*
 * soracom-uptime-lp.ino
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

static constexpr int START_DELAY = 1000 * 10;         // [ms]
static constexpr int MEASURE_PERIOD = 1000 * 60 * 5;  // [ms]
static constexpr int PSM_PERIOD = 60 * 6;             // [s]
static constexpr int PSM_ACTIVE = 2;                  // [s]
static constexpr int POWER_ON_TIMEOUT = 1000 * 20;    // [ms]
static constexpr int RECEIVE_TIMEOUT = 1000 * 10;     // [ms]

#define ABORT_IF_FAILED(result) \
  do { \
    if ((result) != WioCellularResult::Ok) abort(); \
  } while (0)

static SemaphoreHandle_t CellularWorkSem;
static SemaphoreHandle_t CellularStartSem;
static SemaphoreHandle_t MeasureSem;
static QueueSetHandle_t QueueSet;

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

  assert(CellularWorkSem = WioCellular.getInterface().getReceivedNotificationSemaphone());
  assert(CellularStartSem = xSemaphoreCreateBinary());
  assert(MeasureSem = xSemaphoreCreateBinary());

  assert(QueueSet = xQueueCreateSet(3));
  assert(xQueueAddToSet(CellularWorkSem, QueueSet) == pdPASS);
  assert(xQueueAddToSet(CellularStartSem, QueueSet) == pdPASS);
  assert(xQueueAddToSet(MeasureSem, QueueSet) == pdPASS);

  WioCellular.begin();
  ABORT_IF_FAILED(WioCellular.powerOn(POWER_ON_TIMEOUT));

  WioNetwork.config.searchAccessTechnology = SEARCH_ACCESS_TECHNOLOGY;
  WioNetwork.config.ltemBand = LTEM_BAND;
  WioNetwork.config.apn = APN;
  WioNetwork.begin();

  assert(xTimerStart(xTimerCreate("CellularStart", pdMS_TO_TICKS(START_DELAY), pdFALSE, CellularStartSem, semaphoreGiveTimerHandler), 0) == pdPASS);

  digitalWrite(LED_BUILTIN, LOW);
}

void loop(void) {
  Serial.flush();
  const auto activatedMember = xQueueSelectFromSet(QueueSet, portMAX_DELAY);
  assert(activatedMember);

  digitalWrite(LED_BUILTIN, HIGH);

  if (activatedMember == CellularWorkSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    WioCellular.doWork(0);
  } else if (activatedMember == CellularStartSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    ABORT_IF_FAILED(WioCellular.setPsmEnteringIndicationUrc(true));
    ABORT_IF_FAILED(WioCellular.setPsm(1, PSM_PERIOD, PSM_ACTIVE));

    assert(xTimerStart(xTimerCreate("Measure", pdMS_TO_TICKS(MEASURE_PERIOD), pdTRUE, MeasureSem, semaphoreGiveTimerHandler), 0) == pdPASS);
  } else if (activatedMember == MeasureSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    JsonDoc.clear();
    if (measure(JsonDoc)) {
      ABORT_IF_FAILED(WioCellular.powerOn(POWER_ON_TIMEOUT));
      send(JsonDoc);
    }
  }

  digitalWrite(LED_BUILTIN, LOW);
}

static bool measure(JsonDocument& doc) {
  Serial.println("### Measuring");

  doc["uptime"] = millis() / 1000;

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

static void semaphoreGiveTimerHandler(TimerHandle_t timer) {
  xSemaphoreGive(pvTimerGetTimerID(timer));
}

template<typename T>
void printData(T& stream, const void* data, size_t size) {
  auto p = static_cast<const char*>(data);

  for (; size > 0; --size, ++p)
    stream.write(0x20 <= *p && *p <= 0x7f ? *p : '.');
}
