/*
 * WioCellular.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef WIOCELLULAR_HPP
#define WIOCELLULAR_HPP

#ifdef ARDUINO_WIO_BG770A

#define BG770AINTERFACE_VDD_EXT (PIN_VDD_EXT)
#define BG770AINTERFACE_PWRKEY (PIN_PWRKEY)

#define BG770AINTERFACE_MAIN_TXD (PIN_MAIN_TXD)
#define BG770AINTERFACE_MAIN_RXD (PIN_MAIN_RXD)
#define BG770AINTERFACE_MAIN_CTS (PIN_MAIN_CTS)
#define BG770AINTERFACE_MAIN_RTS (PIN_MAIN_RTS)
#define BG770AINTERFACE_MAIN_DTR (PIN_MAIN_DTR)
#define BG770AINTERFACE_MAIN_DCD (PIN_MAIN_DCD)
#define BG770AINTERFACE_MAIN_RI (PIN_MAIN_RI)

#define BG770AINTERFACE_MAIN_UARTE (MAIN_UARTE)
#define BG770AINTERFACE_MAIN_UARTE_IRQn (MAIN_UARTE_IRQn)
#define BG770AINTERFACE_MAIN_UARTE_IRQHANDLER (MAIN_UARTE_IRQHANDLER)

#include <Arduino.h>
#include "module/Bg770aInterface.hpp"
#include "module/Bg770a.hpp"
#include "board/WioBg770a.hpp"

using WioCellularModuleInterface = Bg770aInterface<Uart>;
using WioCellularModule = Bg770a<WioCellularModuleInterface>;
using WioCellularBoard = WioBg770a<WioCellularModule, WioCellularModuleInterface>;

extern WioCellularBoard WioCellular;

#endif

#include "client/WioCellularTcpClient.hpp"

#endif // WIOCELLULAR_HPP
