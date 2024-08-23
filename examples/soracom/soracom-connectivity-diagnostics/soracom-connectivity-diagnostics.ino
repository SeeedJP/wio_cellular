/*
 * soracom-connectivity-diagnostics.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */
/*
 * Connectivity diagnostics for LTE-M Shield for Arduino
 *
 * Copyright SORACOM
 * This software is released under the MIT License, and libraries used by these sketches
 * are subject to their respective licenses.
 * See also: https://github.com/soracom-labs/arduino-dragino-unified/README.md
 */

#include <Adafruit_TinyUSB.h>
#include <WioCellular.h>

static constexpr int POWER_ON_TIMEOUT = 20000;  // [ms]

#define CONSOLE Serial
#define ABORT_IF_FAILED(result) \
  do { \
    if ((result) != WioCellularResult::Ok) abort(); \
  } while (0)

static WioCellularResult queryCommand(const char *command, int timeout) {
  CONSOLE.print("> ");
  CONSOLE.println(command);
  return WioCellular.queryCommand(
    command, [](const std::string &response) -> bool {
      CONSOLE.println(response.c_str());
      return true;
    },
    timeout);
}

static WioCellularResult executeCommand(const char *command, int timeout) {
  CONSOLE.print("> ");
  CONSOLE.println(command);
  const auto result = WioCellular.executeCommand(command, timeout);
  CONSOLE.println(WioCellularResultToString(result));
  return result;
}

static void showModemInformation(void) {
  ABORT_IF_FAILED(queryCommand("AT+GSN", 300));
  ABORT_IF_FAILED(queryCommand("AT+CIMI", 300));
  ABORT_IF_FAILED(queryCommand("AT+QSIMSTAT?", 300));
}

static void showNetworkInformation(void) {
  ABORT_IF_FAILED(queryCommand("AT+QIACT?", 300));
  ABORT_IF_FAILED(queryCommand("AT+QCSQ", 300));
  ABORT_IF_FAILED(queryCommand("AT+COPS?", 300));
  ABORT_IF_FAILED(queryCommand("AT+CGPADDR=1", 300));
}

static bool setupNetworkConfigurations(void) {
  const bool setupPDP = executeCommand("AT+CGDCONT=1,\"IP\",\"soracom.io\",\"0.0.0.0\",0,0,0", 300) == WioCellularResult::Ok;
  const bool networkCategory = executeCommand("AT+QCFG=\"iotopmode\",0,1", 300) == WioCellularResult::Ok;
  const bool scanSequence = executeCommand("AT+QCFG=\"nwscanseq\",00,1", 300) == WioCellularResult::Ok;

  return setupPDP && networkCategory && scanSequence;
}

static int printPingResult(String input) {
  input.replace("+QPING: ", "");

  char buf[100] = { 0 };
  input.toCharArray(buf, 100);
  if (strlen(buf) <= 0) return -1;

  int result = atoi(strtok(buf, ","));
  if (result == 0) {
    CONSOLE.print(F("Dest="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", Bytes="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", Time="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", TTL="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.println();
  } else {
    CONSOLE.print("(R)Error: ");
    CONSOLE.println(result);
  }

  return result;
}

static int printPingSummary(String input) {
  input.replace("+QPING: ", "");

  char buf[100] = { 0 };
  input.toCharArray(buf, 100);
  if (strlen(buf) <= 0) return -1;

  int result = atoi(strtok(buf, ","));
  if (result == 0) {
    CONSOLE.print(F("Sent="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", Received="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", Lost="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", Min="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", Max="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.print(F(", Avg="));
    CONSOLE.print(strtok(NULL, ","));
    CONSOLE.println();
  } else {
    CONSOLE.print("(S)Error: ");
    CONSOLE.println(result);
  }

  return result;
}

static void pingToSoracomNetwork(void) {
  constexpr uint32_t timeout = 15000;

  std::vector<std::string> pingResponse;
  const auto handler = WioCellular.registerUrcHandler([&pingResponse](const std::string &response) -> bool {
    if (response.compare(0, 8, "+QPING: ") == 0) {
      pingResponse.push_back(response);
      return true;
    }
    return false;
  });

  ABORT_IF_FAILED(executeCommand("AT+QPING=1,\"pong.soracom.io\",3,3", 300000));
  const auto start = millis();
  while (pingResponse.size() < 3 + 1) {
    WioCellular.doWork(timeout - (millis() - start));
    if (millis() - start >= timeout) break;
  }

  WioCellular.unregisterUrcHandler(handler);

  // success
  //   +QPING: 0,"100.127.100.127",32,66,64
  //   +QPING: 0,"100.127.100.127",32,80,64
  //   +QPING: 0,"100.127.100.127",32,80,64
  //   +QPING: 0,3,3,0,66,80,75

  // dns failed
  //   +QPING: 565

  // operation timeout
  //   +QPING: 569
  //   +QPING: 569
  //   +QPING: 569
  //   +QPING: 569

  if (pingResponse.size() >= 1) printPingResult(pingResponse[0].c_str());
  if (pingResponse.size() >= 2) printPingResult(pingResponse[1].c_str());
  if (pingResponse.size() >= 3) printPingResult(pingResponse[2].c_str());
  if (pingResponse.size() >= 4) printPingSummary(pingResponse[3].c_str());
}

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

  CONSOLE.println();
  CONSOLE.println("****************************");
  CONSOLE.println("* Connectivity diagnostics *");
  CONSOLE.println("****************************");

  CONSOLE.println();
  CONSOLE.print("--- Initializing modem, please wait for a while...");
  WioCellular.begin();
  ABORT_IF_FAILED(WioCellular.powerOn(POWER_ON_TIMEOUT));
  CONSOLE.println("[OK]");

  CONSOLE.print("Target modem: ");
  std::string revision;
  ABORT_IF_FAILED(WioCellular.getModemInfo(&revision));
  CONSOLE.println(revision.c_str());

  CONSOLE.println();
  CONSOLE.println("--- Getting modem info...");
  showModemInformation();

  CONSOLE.println();
  CONSOLE.println("--- Executing AT commands to connect SORACOM network...");
  bool setupNetworkResult = setupNetworkConfigurations();
  if (!setupNetworkResult) {
    CONSOLE.println("Failed to execute setup commands, please RESET and retry later.");
    while (1)
      ;
  }

  CONSOLE.println();
  CONSOLE.print("--- Connecting to cellular network, please wait for a while...");
  int state;
  const auto start = millis();
  do {
    ABORT_IF_FAILED(WioCellular.getEpsNetworkRegistrationState(&state));
  } while (state != 1 && state != 5 && millis() - start < 120000);
  CONSOLE.println(state == 1 || state == 5 ? "[OK]" : "[FAILED]");
  if (state != 1 && state != 5) {
    CONSOLE.println("Failed to connect cellular network.");
    CONSOLE.println("Make sure active SIM has been inserted to the modem, and then check the antenna has been connected correctly.");
    CONSOLE.println("Please RESET and retry later.");
    while (1)
      ;
  }

  CONSOLE.println();
  CONSOLE.println("--- Getting network info...");
  showNetworkInformation();

  CONSOLE.println();
  CONSOLE.println("--- Conntectivity test: Ping to pong.soracom.io...");
  pingToSoracomNetwork();

  CONSOLE.println();
  CONSOLE.println("--- Execution completed, please write your own sketch and enjoy it.");
}

void loop(void) {
  delay(1000);
}
