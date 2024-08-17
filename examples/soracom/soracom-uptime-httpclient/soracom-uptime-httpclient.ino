/*
 * soracom-uptime-httpclient.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   http://librarymanager#ArduinoJson 7.0.4
//   http://librarymanager#ArduinoHttpClient 0.6.1

#include <Adafruit_TinyUSB.h>
#include <algorithm>
#include <map>
#include <vector>
#include <WioCellular.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>

static const char APN[] = "soracom.io";
static const char HOST[] = "metadata.soracom.io";
static const char PATH[] = "/v1/subscriber/tags";
static constexpr int PORT = 80;

static constexpr int INTERVAL = 1000 * 60 * 15;  // [ms]
static constexpr int POWER_ON_TIMEOUT = 20000;   // [ms]
static constexpr int RECEIVE_TIMEOUT = 10000;    // [ms]

#define ABORT_IF_FAILED(result) \
  do { \
    if ((result) != WioCellularResult::Ok) abort(); \
  } while (0)

struct HttpResponse {
  int statusCode;
  std::map<std::string, std::string> headers;
  std::string body;
};

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

  setupCellular();

  digitalWrite(LED_BUILTIN, LOW);
}

void loop(void) {
  digitalWrite(LED_BUILTIN, HIGH);

  JsonDoc.clear();
  if (generateRequestBody(JsonDoc)) {
    std::string jsonStr;
    serializeJson(JsonDoc, jsonStr);
    Serial.println(jsonStr.c_str());

    HttpResponse response = httpRequest(TcpClient, HOST, PORT, PATH, "PUT", "application/json", jsonStr.c_str());

    Serial.println("Header(s):");
    for (auto header : response.headers) {
      Serial.print("  ");
      Serial.print(header.first.c_str());
      Serial.print(" : ");
      Serial.print(header.second.c_str());
      Serial.println();
    }
    Serial.print("Body: ");
    Serial.println(response.body.c_str());

    if (response.statusCode == 200) {
      JsonDoc.clear();
      deserializeJson(JsonDoc, response.body.c_str());
      // Output the IMSI field as an example of how to use the response
      Serial.print("Response imsi> ");
      Serial.print(JsonDoc["imsi"].as<String>());
      Serial.println();
    }
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

/**
 * Generate request body for update SIM tag
 * See: https://users.soracom.io/ja-jp/docs/air/use-metadata/#%E3%83%A1%E3%82%BF%E3%83%87%E3%83%BC%E3%82%BF%E3%81%AE%E6%9B%B8%E3%81%8D%E8%BE%BC%E3%81%BF%E4%BE%8B-iot-sim-%E3%81%AE%E3%82%BF%E3%82%B0%E3%82%92%E8%BF%BD%E5%8A%A0--%E6%9B%B4%E6%96%B0%E3%81%99%E3%82%8B
 */
static bool generateRequestBody(JsonDocument& doc) {
  Serial.println("### Measuring");

  JsonArray rootArray = doc.to<JsonArray>();

  JsonObject uptimeTag = rootArray.add<JsonObject>();
  uptimeTag["tagName"] = "UPTIME";
  uptimeTag["tagValue"] = std::to_string(millis() / 1000);

  Serial.println("### Completed");

  return true;
}

static HttpResponse httpRequest(Client& client, const char* host, int port, const char* path, const char* method, const char* contentType, const char* requestBody) {
  HttpResponse httpResponse;
  Serial.print("### Requesting to [");
  Serial.print(host);
  Serial.println("]");

  HttpClient httpClient(client, host, port);
  int err = httpClient.startRequest(path, method, contentType, strlen(requestBody), (const byte*)requestBody);
  if (err != 0) {
    httpClient.stop();
    httpResponse.statusCode = err;
    return httpResponse;
  }

  int statusCode = httpClient.responseStatusCode();
  if (!statusCode) {
    httpClient.stop();
    httpResponse.statusCode = statusCode;
    return httpResponse;
  }

  Serial.print("Status code returned ");
  Serial.println(statusCode);
  httpResponse.statusCode = statusCode;

  while (httpClient.headerAvailable()) {
    String headerName = httpClient.readHeaderName();
    String headerValue = httpClient.readHeaderValue();
    httpResponse.headers[headerName.c_str()] = headerValue.c_str();
  }

  int length = httpClient.contentLength();
  if (length >= 0) {
    Serial.print("Content length: ");
    Serial.println(length);
  }
  if (httpClient.isResponseChunked()) {
    Serial.println("The response is chunked");
  }

  String responseBody = httpClient.responseBody();
  httpResponse.body = responseBody.c_str();

  httpClient.stop();

  Serial.println("### End HTTP request");
  Serial.println();

  return httpResponse;
}
