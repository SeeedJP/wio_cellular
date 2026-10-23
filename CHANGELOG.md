# 0.3.16 (2026/10/23)

* SeeedJP nRF52 Boards is 1.5.1

## What's Changed

* examples
  * Add Grove magnetic switch example
  * Add Grove DHT11 temperature and humidity example

# 0.3.15 (2026/5/1)

* SeeedJP nRF52 Boards is 1.5.0

## What's Changed

* module
  * Support Wio BG770A v1.1
* examples
  * vsys: Add example of measuring VSYS

# 0.3.14 (2026/4/15)

* SeeedJP nRF52 Boards is 1.5.0

## What's Changed

* module
  * Remove the retransmission logic from sendSocket2()
* examples
  * soracom-uptime-psm: Add PSM status log to soracom-uptime-psm example

## New Contributors

* @nmikuni Thanks for pull request. 

# 0.3.13 (2025/9/3)

* SeeedJP nRF52 Boards is 1.5.0

## What's Changed

* module
  * 🕷️**Bugfix** Fix no timeout does not work #57
* examples
  * Change LTEM_BAND in examples
  * soracom-uptime-httpclient: Set timeout in HttpClient
  * soracom-uptime-httpclient: Simplify soracom-uptime-httpclient
  * soracom-uptime-off,soracom-uptime-psm: Support PlatformIO for soracom-uptime-off and soracom-uptime-psm
* internal
  * Improved log output code
  * Remove VgroveEnable_ variable
  * Add pio build job to GitHub Actions

# 0.3.12 (2025/6/13)

* SeeedJP nRF52 Boards is 1.4.6

## What's Changed

* examples
  * [soracom-gps-tracker] Change TCP to UDP
* client
  * Add UDP client feature (WioCellularUdpClient2 class)
  * 🔥Rename waitforConnect() to waitForConnect()
* module
  * Add getSignalingConnectionStatus() and setSignalingConnectionStatus()

# 0.3.11 (2025/6/11)

* SeeedJP nRF52 Boards is 1.4.6

## What's Changed

* examples
  * Fix warnings
  * [watchdog] Display reset reason
  * [soracom-gps-tracker] Suspend all tasks when abort
  * [soracom-gps-tracker] Add watchdog function
  * [soracom-gps-tracker] Add logic to stop abnormal reboot
  * Update abortHandler()
* component
  * Add NonVolatileMemory doxygen
* all
  * Change "(void)" to "()"

# 0.3.10 (2025/5/29)

* SeeedJP nRF52 Boards is 1.4.6

## What's Changed

* examples
  * Use VGROVE_ENABLE_ON/OFF
  * NETWORK_TIMEOUT changed to 3 min. #52 
  * 🔥Remove soracom-uptime-lp.ino
  * 🕷️**Bugfix** Fixed a bug that caused the log output of sent data to be truncated
  * 🕷️**Bugfix** Fixed a bug that data with time of 0 was recorded
* board
  * 🔥Remove enableCellularPower() and disableCellularPower()
* module
  * Add setOperator()
  * 🔥Remove Bg770aTcpipCommands class
* component
  * Add NonVolatileMemory component

# 0.3.9 (2025/5/15)

## What's Changed

* examples
  * 🕷️**Bugfix** Fixed forgotten call to WioNetwork.end()
  * 🕷️**Bugfix** Add delay after writing to PIN_VGROVE_ENABLE #49 
* module
  * Add getBatteryChargeState() and getFlowControl(), setFlowControl()
  * Change "Enable Hardware Flow Control" sequence
  * Remove "Enable sleep mode" at powerOn()
  * Log elapsed time from ECO to FRC

## Contributors

* @PikaPikaLight reported an important issue #48 . Thank you very much.

# 0.3.8 (2025/4/18)

## What's Changed

* module
  * Add getClock()
  * Change AT+QISENDEX to AT+QISEND at sendSocket2() #32 #46 
* examples
  * Add comment #45 
  * Change soracom-gps-tracker to batch write

# 0.3.7 (2025/3/17)

## What's Changed

* module
  * 🕷️**Bugfix** Fix return value of WioCellularArduinoTcpClient.available() and WioCellularArduinoTcpClient.read() #43 

# 0.3.6 (2025/2/25)

## What's Changed

* module
  * Support for S3 delimiter #34 
  * Deprecated enableGrovePower() and disableGrovePower()
* examples
  * Update soracom-gps-tracker

# 0.3.5 (2025/2/20)

## What's Changed

* module
  * 🕷️**First-aid** Data received by WioCellularTcpClient2.receive is wrong #41 

# 0.3.4 (2025/2/7)

## What's Changed

* examples
  * Remove ABORT_IF_FAILED()
  * Set WioNetwork.config first
* module
  * Add wait for active at powerOn() #37
  * Add reset when RdyTimeout at powerOn() #36

## New Contributors

* @susami-lab has contributed greatly to this renovation and testing. Thank you very much.

# 0.3.3 (2025/1/31)

## What's Changed

* module
  * Add commandTimeoutMin and commandEchoTimeout at Bg770a #35 
  * Change AT response delimiter #34 

# 0.3.2 (2025/1/30)

## What's Changed

* general
  * Change AT+QISEND to AT+QISENDEX #32
* network
  * Add Bg770aNetwork.end() #31 
* examples
  * Add soracom-uptime-off.ino
  * Add soracom-uptime-psm.ino #27 

# 0.3.1 (2025/1/23)

## What's Changed

* examples
  * Added cellular/cellular-mqtt-pubsubclient

## New Contributors

* @Miura55 made their first contribution in https://github.com/SeeedJP/wio_cellular/pull/29

# 0.3.0 (2025/1/20)

## What's Changed

* general
  * 🔥Reconstruct Socket and TcpClient
* network
  * Add Bg770aNetwork.waitUntilCommunicationAvailable()

# 0.2.1 (2025/1/17)

## What's Changed

* general
  * Flush USB-CDC in doWork(), doWorkUntil()
* client
  * Speed up WioCellularTcpClient.available()
* module
  * Add AtClient.registerUrcHandler2()
  * Add AtClient.doWork(int, const std::function<bool(void)> &)
  * 🕷️**Bugfix** Fix timeout logic
* examples
  * USER LED flashes when abort

# 0.2.0 (2024/12/17)

## What's Changed

* network
  * Added NOSET to SearchAccessTechnology
  * 🔥Change default value of Bg770aNetwork.config
    * SearchAccessTechnology = LTEM_NBIOT -> NOSET
    * ltemBand = "0x2000000000f0e189f" -> ""
    * nbiotBand = "0x200000000090f189f" -> ""
* examples
  * Added cellular/cellular-status

# 0.1.12 (2024/12/12)

## What's Changed

* examples
  * Fix for PlatformIO
  * Change to readable

# 0.1.11 (2024/10/16)

# 0.1.10 (2024/10/2)

## What's Changed

* Update many examples
* network
  * Add Bg770aNetwork class and WioNetwork instance
* module
  * Bg770a: Add functions
    * getPhoneFunctionality()
    * getSocketUnusedConnectId()
  * Bg770a: Add constants
    * RECEIVE_SOCKET_SIZE_MAX
  * Bg770a: Fix timeout value at setPhoneFunctionality()
  * Bg770a: Increase echo timeout
  * Bg770a: Increase timeout at setSearchFrequencyBand()
  * AtClient: Add functions
    * doWorkUntil()

# 0.1.9 (2024/9/6)

## What's Changed

* examples
  * shell: Bugfix #13 
* module
  * Bg770a: Refactoring

# 0.1.8 (2024/9/5)

## What's Changed

* examples
  * shell: Update info and status commands
  * watchdog: Support for SeeedJP nRF52 Boards 1.3.0
* module
  * Bg770a: Refactoring
  * Bg770a: Insufficient parsing on some commands
  * Bg770a: Add argument assertion
  * Bg770a: receiveSocket() discards data when data is nullptr
* Move few classes into namespace

# 0.1.7 (2024/8/30)

## What's Changed

* examples
  * Add soracom-uptime-tcpclient ( Thanks @sayacom )
  * Support for PlatformIO
  * soracom-connectivity-diagnostics: Increased network attach timeout (Thanks @yuta-imai )
  * shell: Add items to "info" command
  * shell: Add items to "status" command
* module
  * Bg770a: Add functions
    * getSimInitializationStatus()
    * getSimInsertionStatus()
    * getSignalQuality()
    * getSearchFrequencyBand()
    * setSearchFrequencyBand()
    * getSearchAccessTechnology()
    * setSearchAccessTechnology()
    * getSearchAccessTechnologySequence()
    * setSearchAccessTechnologySequence()

# 0.1.6 (2024/8/8)

# 0.1.5 (2024/8/7)

# 0.1.4 (2024/7/25)

# 0.1.3 (2024/7/19)
