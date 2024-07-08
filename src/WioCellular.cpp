/*
 * WioCellular.cpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifdef ARDUINO_WIO_BG770A

#include "WioCellular.hpp"

////////////////////////////////////////////////////////////////////////////////
// WioCellularModuleInterfaceInstance

static WioCellularModuleInterface WioCellularModuleInterfaceInstance;

extern "C" void BG770AINTERFACE_MAIN_UARTE_IRQHANDLER(void)
{
    WioCellularModuleInterfaceInstance.mainUartIrqHandler();
}

void BG770AINTERFACE_VDD_EXT_IRQHANDLER(void)
{
    WioCellularModuleInterfaceInstance.vddExtHandler();
}

////////////////////////////////////////////////////////////////////////////////
// WioCellular

WioCellularBoard WioCellular{WioCellularModuleInterfaceInstance};

#endif // ARDUINO_WIO_BG770A
