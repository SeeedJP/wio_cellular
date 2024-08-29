/*
 * shell.ino
 * Copyright (C) Seeed K.K.
 * MIT License
 */

////////////////////////////////////////////////////////////////////////////////
// Libraries:
//   https://github.com/matsujirushi/ntshell 0.3.1

#include <Adafruit_TinyUSB.h>
#include <algorithm>
#include <nrfx_power.h>
#include <ntshell.h>     // Natural Tiny Shell
#include <util/ntopt.h>  // Natural Tiny Shell
#include <WioCellular.h>

static const char APN[] = "soracom.io";

static constexpr int POWER_ON_TIMEOUT = 20000;  // [ms]

static constexpr int PDP_CONTEXT_ID = 1;
static constexpr int SOCKET_ID = 0;

static ntshell_t Shell;

typedef int (*CommandHandlerType)(int argc, char **argv);

struct CommandType {
  const char *Command;
  const char *Args;
  const char *Description;
  CommandHandlerType CommandHandler;
};

static int CommandInfo(int argc, char **argv);
static int CommandStatus(int argc, char **argv);
static int CommandPdpContext(int argc, char **argv);
static int CommandSocket(int argc, char **argv);
static int CommandSocketOpen(int argc, char **argv);
static int CommandSocketSend(int argc, char **argv);
static int CommandSocketReceive(int argc, char **argv);
static int CommandSocketSendReceive(int argc, char **argv);
static int CommandSocketClose(int argc, char **argv);
static int CommandSleep(int argc, char **argv);
static int CommandWakeup(int argc, char **argv);
static int CommandEdrx(int argc, char **argv);
static int CommandPsm(int argc, char **argv);
static int CommandResetAllSettings(int argc, char **argv);
static int CommandTransparent(int argc, char **argv);
static int CommandHelp(int argc, char **argv);

static const CommandType CommandList[] = {
  { "info", "", "Display cellular information.", CommandInfo },
  { "status", "", "Display cellular statuses.", CommandStatus },
  { "pdpctx", "", "Display PDP contexts.", CommandPdpContext },
  { "socket", "", "Display socket connection statuses.", CommandSocket },
  { "socketopen", "TCP <host> <port>", "Connect to host.", CommandSocketOpen },
  { "socketsend", "<payload>", "Send.", CommandSocketSend },
  { "socketreceive", "", "Receive.", CommandSocketReceive },
  { "socketsendreceive", "<payload>", "Send and receive.", CommandSocketSendReceive },
  { "socketclose", "", "Close socket.", CommandSocketClose },
  { "sleep", "", "[experimental] Sleep.", CommandSleep },
  { "wakeup", "", "[experimental] Wakeup.", CommandWakeup },
  { "edrx", "", "[experimental] Enable eDRX.", CommandEdrx },
  { "psm", "", "[experimental] Enable PSM.", CommandPsm },
  { "resetallsettings", "", "Reset all settings in cellular module.", CommandResetAllSettings },
  { "transparent", "", "Transparent connection between console and cellular module. Click USER button to return to prompt.", CommandTransparent },
  { "help", "", "Display command description.", CommandHelp },
};

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

  ntshell_init(&Shell, shellRead, shellWrite, shellCallback, nullptr);
  ntshell_set_prompt(&Shell, "\nWIO>");

  Serial.printf("Initialize\n");
  WioCellular.begin();

  Serial.printf("Turn on cellular\n");
  {
    const auto start = millis();
    if (WioCellular.powerOn(POWER_ON_TIMEOUT) != WioCellularResult::Ok) {
      Serial.printf("ERROR\n");
      abort();
    }
    Serial.printf("... %lu[ms]\n", millis() - start);
  }

  Serial.printf("Set URC settings\n");
  WioCellular.setEpsNetworkRegistrationStatusUrc(2);

  Serial.printf("Check PDP context\n");
  std::vector<WioCellularModule::PdpContext> pdpContexts;
  WioCellularResult result;
  if ((result = WioCellular.getPdpContext(&pdpContexts)) != WioCellularResult::Ok) {
    Serial.printf("ERROR: %d\n", static_cast<int>(result));
    abort();
  }

  if (std::find_if(pdpContexts.begin(), pdpContexts.end(), [](const WioCellularModule::PdpContext &pdpContext) {
        return pdpContext.apn == APN;
      })
      == pdpContexts.end()) {
    Serial.printf("Set PDP context\n");
    WioCellular.setPhoneFunctionality(0);
    if ((result = WioCellular.setPdpContext({ PDP_CONTEXT_ID, "IP", APN, "0.0.0.0", 0, 0, 0 })) != WioCellularResult::Ok) {
      Serial.printf("ERROR: %d\n", static_cast<int>(result));
      abort();
    }
    WioCellular.setPhoneFunctionality(1);
  }
}

void loop(void) {
  WioCellular.doWork(10);  // Spin
  ntshell_execute_nb(&Shell);
}

static int shellRead(char *buf, int cnt, void *extobj) {
  for (int i = 0; i < cnt; ++i) {
    const auto c = Serial.read();
    if (c < 0) {
      return i;
    }
    buf[i] = c;
  }

  return cnt;
}

static int shellWrite(const char *buf, int cnt, void *extobj) {
  return Serial.write(buf, cnt);
}

static int shellCallback(const char *text, void *extobj) {
  return ntopt_parse(text, commandCallback, extobj);
}

static int commandCallback(int argc, char **argv, void *extobj) {
  if (argc < 1) {
    return 0;
  }

  for (size_t i = 0; i < std::extent<decltype(CommandList)>::value; ++i) {
    if (strcmp(argv[0], CommandList[i].Command) == 0) {
      return CommandList[i].CommandHandler(argc, argv);
    }
  }

  Serial.println("Unknown command found.");
  return 0;
}

static int CommandInfo(int argc, char **argv) {
  std::string imei;
  WioCellular.getIMEI(&imei);
  std::string revision;
  WioCellular.getModemInfo(&revision);
  int simInserted;
  WioCellular.getSimInsertionStatus(nullptr, &simInserted);
  int simInitStatus;
  WioCellular.getSimInitializationStatus(&simInitStatus);
  std::string simState;
  WioCellular.getSimState(&simState);
  std::string imsi;
  WioCellular.getIMSI(&imsi);
  std::string iccid;
  WioCellular.getSimCCID(&iccid);
  std::string phoneNumber;
  WioCellular.getPhoneNumber(&phoneNumber);
  int searchAct;
  WioCellular.getSearchAccessTechnology(&searchAct);
  std::string searchActSeq;
  WioCellular.getSearchAccessTechnologySequence(&searchActSeq);
  std::string gsmBand;
  std::string emtcBand;
  std::string nbiotBand;
  WioCellular.getSearchFrequencyBand(&gsmBand, &emtcBand, &nbiotBand);

  Serial.printf("IMEI:                 %s\n", imei.c_str());
  Serial.printf("Revision:             %s\n", revision.c_str());
  Serial.printf("SIM Inserted:         %s\n", simInserted == 0 ? "No" : simInserted == 1 ? "Yes"
                                                                                         : "Unknown");
  Serial.printf("SIM Init:             %s\n", simInitStatus == 0 ? "Initial" : simInitStatus == 1 ? "CPIN Ready"
                                                                             : simInitStatus == 2 ? "SMS Done"
                                                                             : simInitStatus == 3 ? "CPIN Ready & SMS Done"
                                                                                                  : "Unknown");
  Serial.printf("SIM State:            %s\n", simState.c_str());
  Serial.printf("IMSI:                 %s\n", imsi.c_str());
  Serial.printf("ICCID:                %s\n", iccid.c_str());
  Serial.printf("Phone Number:         %s\n", phoneNumber.c_str());
  Serial.printf("Search ACT:           %s\n", searchAct == 0 ? "eMTC" : searchAct == 1 ? "NB-IoT"
                                                                      : searchAct == 2 ? "eMTC and NB-IoT"
                                                                                       : "Unknown");
  Serial.printf("Search ACT Sequence:  %s\n", searchActSeq == "0203" ? "eMTC -> NB-IoT" : searchActSeq == "0302" ? "NB-IoT -> eMTC"
                                                                                                                 : "Unknown");
  Serial.printf("Search Band - eMTC:   %s\n", emtcBand.c_str());
  Serial.printf("Search Band - NB-IoT: %s\n", nbiotBand.c_str());

  return 0;
}

static int CommandStatus(int argc, char **argv) {
  int rssi;
  int ber;
  WioCellular.getSignalQuality(&rssi, &ber);
  int state;
  WioCellular.getEpsNetworkRegistrationState(&state);
  int mode;
  int format;
  std::string oper;
  int act;
  WioCellular.getOperator(&mode, &format, &oper, &act);
  int psState;
  WioCellular.getPacketDomainState(&psState);

  Serial.printf("RSSI:                %d\n", rssi);
  Serial.printf("BER:                 %d\n", ber);
  Serial.printf("Registration State:  %d\n", state);
  Serial.printf("Operator:            %s, %d\n", oper.c_str(), act);
  Serial.printf("Packet Domain State: %d\n", psState);

  return 0;
}

static int CommandPdpContext(int argc, char **argv) {
  std::vector<WioCellularModule::PdpContext> contexts;
  WioCellular.getPdpContext(&contexts);
  std::vector<WioCellularModule::PdpContextStatus> statuses;
  WioCellular.getPdpContextStatus(&statuses);

  Serial.printf("PDP Contexts:\n");
  for (const auto &context : contexts) {
    Serial.printf(" [%d]: %s %s %s %d %d %d\n", context.cid, context.pdpType.c_str(), context.apn.c_str(), context.pdpAddr.c_str(), context.dComp, context.hComp, context.ipV4AddrAlloc);
  }
  Serial.printf("PDP Context Statuses:\n");
  for (const auto &status : statuses) {
    Serial.printf(" [%d]: %d\n", status.cid, status.state);
  }

  return 0;
}

static int CommandSocket(int argc, char **argv) {
  std::vector<WioCellularModule::SocketStatus> statuses;
  WioCellular.getSocketStatus(PDP_CONTEXT_ID, &statuses);

  Serial.printf("Socket Statuses:\n");
  for (const auto &status : statuses) {
    Serial.printf(" [%d]: %s %s %d %d %d\n", status.connectId, status.serviceType.c_str(), status.ipAddress.c_str(), status.remotePort, status.localPort, status.socketState);
  }

  return 0;
}

static int CommandSocketOpen(int argc, char **argv) {
  if (argc != 4) {
    Serial.printf("Wrong number of arguments.\n");
    return 1;
  }

  if (strcmp(argv[1], "TCP") != 0) {
    Serial.printf("Unknown type.\n");
    return 1;
  }

  WioCellular.openSocket(PDP_CONTEXT_ID, SOCKET_ID, argv[1], argv[2], atoi(argv[3]), 0);

  return 0;
}

static int CommandSocketSend(int argc, char **argv) {
  if (argc != 2) {
    Serial.printf("Wrong number of arguments.\n");
    return 1;
  }

  WioCellular.sendSocket(SOCKET_ID, argv[1]);

  return 0;
}

static int CommandSocketReceive(int argc, char **argv) {
  size_t available;
  WioCellular.getSocketReceiveAvailable(SOCKET_ID, &available);
  if (available >= 1) {
    char data[available];
    size_t dataSize;
    WioCellular.receiveSocket(SOCKET_ID, data, sizeof(data), &dataSize);
    Serial.printf("%d: ", dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
      Serial.printf("%02X ", data[i]);
    }
    Serial.printf("\n");
  }

  return 0;
}

static int CommandSocketSendReceive(int argc, char **argv) {
  if (argc != 2) {
    Serial.printf("Wrong number of arguments.\n");
    return 1;
  }

  WioCellular.sendSocket(SOCKET_ID, argv[1]);

  static char data[1500];
  size_t dataSize;
  if (WioCellular.receiveSocket(SOCKET_ID, data, sizeof(data), &dataSize, 5000) != WioCellularResult::Ok) {
    Serial.printf("RECEIVE ERROR\n");
    return 0;
  }
  Serial.printf("%d: ", dataSize);
  for (size_t i = 0; i < dataSize; ++i) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.printf("\n");

  return 0;
}

static int CommandSocketClose(int argc, char **argv) {
  WioCellular.closeSocket(SOCKET_ID);

  return 0;
}

static int CommandSleep(int argc, char **argv) {
  WioCellular.getInterface().sleep();

  return 0;
}

static int CommandWakeup(int argc, char **argv) {
  WioCellular.getInterface().wakeup();

  return 0;
}

static int CommandEdrx(int argc, char **argv) {
  WioCellular.setEdrx(2, 4, 4);  // eMTC, PCL=61.44[sec.]
  CommandSleep(0, nullptr);

  return 0;
}

static int CommandPsm(int argc, char **argv) {
  WioCellular.setPsmEnteringIndicationUrc(true);
  WioCellular.setPsm(1, 120, 16);  // T3412=2[min.], T3324=16[sec.]

  return 0;
}

static int CommandResetAllSettings(int argc, char **argv) {
  Serial.printf("Resetting ...\n");
  {
    const auto start = millis();
    if (WioCellular.factoryDefault(30000) != WioCellularResult::Ok) {
      Serial.printf("ERROR\n");
      abort();
    }
    Serial.printf("... %lu[ms]\n", millis() - start);
  }

  Serial.printf("Done.\n");

  return 0;
}

static int CommandTransparent(int argc, char **argv) {
  int c;

  while (true) {
    if (digitalRead(PIN_BUTTON1) == LOW) {
      return 0;
    }

    while ((c = Serial.read()) >= 0) {
      WioCellular.getInterface().write(c);
    }

    while ((c = WioCellular.getInterface().read()) >= 0) {
      Serial.write(c);
    }

    delay(2);
  }
}

static int CommandHelp(int argc, char **argv) {
  for (const auto command : CommandList) {
    Serial.printf("%-20s : %-20s : %s\n", command.Command, command.Args, command.Description);
  }

  return 0;
}
