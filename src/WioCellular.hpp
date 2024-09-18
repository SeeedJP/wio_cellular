/*
 * WioCellular.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef WIOCELLULAR_HPP
#define WIOCELLULAR_HPP

#ifdef ARDUINO_WIO_BG770A

// TODO I want to remove the define, but I don't know what code to use. (X_X)
#define BG770AINTERFACE_MAIN_UARTE (MAIN_UARTE)
#define BG770AINTERFACE_MAIN_UARTE_IRQn (MAIN_UARTE_IRQn)
#define BG770AINTERFACE_MAIN_UARTE_IRQHANDLER (MAIN_UARTE_IRQHANDLER)

#include <Arduino.h>

struct Bg770aInterfaceConstant
{
    static constexpr int VDD_EXT_PIN = PIN_VDD_EXT;
    static constexpr int PWRKEY_PIN = PIN_PWRKEY;
#ifdef PIN_RESET_N
    static constexpr int RESET_N_PIN = PIN_RESET_N;
#endif // PIN_RESET_N

    static constexpr int MAIN_TXD_PIN = PIN_MAIN_TXD;
    static constexpr int MAIN_RXD_PIN = PIN_MAIN_RXD;
    static constexpr int MAIN_CTS_PIN = PIN_MAIN_CTS;
    static constexpr int MAIN_RTS_PIN = PIN_MAIN_RTS;
    static constexpr int MAIN_DTR_PIN = PIN_MAIN_DTR;
    static constexpr int MAIN_DCD_PIN = PIN_MAIN_DCD;
    static constexpr int MAIN_RI_PIN = PIN_MAIN_RI;
};

#include "module/bg770a/Bg770aInterface.hpp"
#include "module/bg770a/Bg770a.hpp"
#include "board/WioBg770a.hpp"

using WioCellularModuleInterface = wiocellular::module::bg770a::Bg770aInterface<Bg770aInterfaceConstant, Uart>;
using WioCellularModule = wiocellular::module::bg770a::Bg770a<WioCellularModuleInterface>;
using WioCellularBoard = wiocellular::board::WioBg770a<WioCellularModule, WioCellularModuleInterface>;

extern WioCellularBoard WioCellular;

#include "network/Bg770aNetwork.hpp"

using WioCellularNetwork = wiocellular::network::Bg770aNetwork;

extern WioCellularNetwork WioNetwork;

#endif

#include "client/WioCellularTcpClient.hpp"

#endif // WIOCELLULAR_HPP
