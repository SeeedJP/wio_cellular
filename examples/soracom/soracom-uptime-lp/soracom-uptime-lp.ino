/*
 * soracom-uptime-lp.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   http://librarymanager#ArduinoJson 7.0.4

#include <Adafruit_TinyUSB.h>
#include <algorithm>
#include <malloc.h>
#include <WioCellular.h>
#include <ArduinoJson.h>

static const char APN[] = "soracom.io";
static const char HOST[] = "uni.soracom.io";
static constexpr int PORT = 23080;

static constexpr int HARTBEAT_PERIOD = 1000 * 10;     // [ms]
static constexpr int DIAG_PERIOD = 1000 * 60 * 60;    // [ms]
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

static constexpr int PDP_CONTEXT_ID = 1;
static constexpr int SOCKET_ID = 0;

static SemaphoreHandle_t CellularWorkSem;
static SemaphoreHandle_t ButtonSem;
static SemaphoreHandle_t HartbeatSem;
static SemaphoreHandle_t DiagSem;
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
  assert(ButtonSem = xSemaphoreCreateBinary());
  assert(HartbeatSem = xSemaphoreCreateBinary());
  assert(DiagSem = xSemaphoreCreateBinary());
  assert(CellularStartSem = xSemaphoreCreateBinary());
  assert(MeasureSem = xSemaphoreCreateBinary());

  assert(QueueSet = xQueueCreateSet(6));
  assert(xQueueAddToSet(CellularWorkSem, QueueSet) == pdPASS);
  assert(xQueueAddToSet(ButtonSem, QueueSet) == pdPASS);
  assert(xQueueAddToSet(HartbeatSem, QueueSet) == pdPASS);
  assert(xQueueAddToSet(DiagSem, QueueSet) == pdPASS);
  assert(xQueueAddToSet(CellularStartSem, QueueSet) == pdPASS);
  assert(xQueueAddToSet(MeasureSem, QueueSet) == pdPASS);

  WioCellular.begin();
  ABORT_IF_FAILED(WioCellular.powerOn(POWER_ON_TIMEOUT));

  setupCellular();

  attachInterrupt(
    PIN_BUTTON1, [](void) {
      assert(xSemaphoreGive(ButtonSem) == pdTRUE);
    },
    FALLING);
  assert(xTimerStart(xTimerCreate("Hartbeat", pdMS_TO_TICKS(HARTBEAT_PERIOD), pdTRUE, HartbeatSem, semaphoreGiveTimerHandler), 0) == pdPASS);
  assert(xTimerStart(xTimerCreate("Diag", pdMS_TO_TICKS(DIAG_PERIOD), pdTRUE, DiagSem, semaphoreGiveTimerHandler), 0) == pdPASS);
  assert(xTimerStart(xTimerCreate("CellularStart", pdMS_TO_TICKS(START_DELAY), pdFALSE, CellularStartSem, semaphoreGiveTimerHandler), 0) == pdPASS);

  digitalWrite(LED_BUILTIN, LOW);
}

void loop(void) {
  const auto activatedMember = xQueueSelectFromSet(QueueSet, portMAX_DELAY);
  assert(activatedMember);

  digitalWrite(LED_BUILTIN, HIGH);

  if (activatedMember == CellularWorkSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    WioCellular.doWork(0);
  } else if (activatedMember == ButtonSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    Serial.println("USER button pressed");
  } else if (activatedMember == HartbeatSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    delay(50);
  } else if (activatedMember == DiagSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    Serial.println("==========");

    // Stack
    const auto taskNumber = uxTaskGetNumberOfTasks();
    TaskStatus_t* taskStatuses = reinterpret_cast<TaskStatus_t*>(pvPortMalloc(sizeof(TaskStatus_t) * taskNumber));
    assert(uxTaskGetSystemState(taskStatuses, taskNumber, nullptr) == taskNumber);
    for (size_t i = 0; i < taskNumber; ++i) {
      Serial.printf("stack_hwm %-7s %u\n", taskStatuses[i].pcTaskName, static_cast<unsigned>(taskStatuses[i].usStackHighWaterMark));
    }
    vPortFree(taskStatuses);

    // Heap
    struct mallinfo info = mallinfo();
    Serial.printf("heap_used %u\n", info.uordblks);

    Serial.println("==========");
  } else if (activatedMember == CellularStartSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    ABORT_IF_FAILED(WioCellular.setPsmEnteringIndicationUrc(true));
    ABORT_IF_FAILED(WioCellular.setPsm(1, PSM_PERIOD, PSM_ACTIVE));

    assert(xTimerStart(xTimerCreate("Measure", pdMS_TO_TICKS(MEASURE_PERIOD), pdTRUE, MeasureSem, semaphoreGiveTimerHandler), 0) == pdPASS);
  } else if (activatedMember == MeasureSem) {
    assert(xSemaphoreTake(activatedMember, 0) == pdTRUE);

    JsonDoc.clear();
    if (measure(JsonDoc)) {
      std::string jsonStr;
      serializeJson(JsonDoc, jsonStr);

      ABORT_IF_FAILED(WioCellular.powerOn(POWER_ON_TIMEOUT));
      send(reinterpret_cast<const uint8_t*>(jsonStr.data()), jsonStr.size());
    }
  }

  digitalWrite(LED_BUILTIN, LOW);
}

static void setupCellular(void) {
  Serial.println("### Setup cellular");

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

static void semaphoreGiveTimerHandler(TimerHandle_t timer) {
  xSemaphoreGive(pvTimerGetTimerID(timer));
}

template<typename T>
void printData(T& stream, const void* data, size_t size) {
  auto p = static_cast<const char*>(data);

  for (; size > 0; --size, ++p)
    stream.write(0x20 <= *p && *p <= 0x7f ? *p : '.');
}
