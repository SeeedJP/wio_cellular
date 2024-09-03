/*
 * Bg770aInterface.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770AINTERFACE_HPP
#define BG770AINTERFACE_HPP

#include <Arduino.h>
#include "Suli3.hpp"

void BG770AINTERFACE_VDD_EXT_IRQHANDLER(void);

namespace wiocellular
{
    namespace module
    {
        namespace bg770a
        {

            /**
             * @~Japanese
             * @brief Quectel BG770Aインターフェース
             *
             * @tparam T MainUARTの実クラス
             *
             * Quectel BG770Aとやりとりするインターフェースのクラスです。
             */
            template <typename UART>
            class Bg770aInterface
            {
            private:
                suli3::arduino::DigitalInputPin<BG770AINTERFACE_VDD_EXT> VddExt_;
                suli3::arduino::DigitalOutputPin<BG770AINTERFACE_PWRKEY> Pwrkey_;
#ifdef BG770AINTERFACE_RESET_N
                suli3::arduino::DigitalOutputPin<BG770AINTERFACE_RESET_N> ResetN_;
#endif // BG770AINTERFACE_RESET_N

                SemaphoreHandle_t MainUartReceived_;  // FreeRTOS
                SemaphoreHandle_t MainUartReceived2_; // FreeRTOS
                UART RealMainUart_;
                suli3::arduino::Uart<decltype(RealMainUart_)> MainUart_;
                suli3::arduino::DigitalOutputPin<BG770AINTERFACE_MAIN_DTR> MainDtr_;
                suli3::arduino::DigitalInputPin<BG770AINTERFACE_MAIN_DCD> MainDcd_;
                suli3::arduino::DigitalInputPin<BG770AINTERFACE_MAIN_RI> MainRi_;

            public:
                /**
                 * @~Japanese
                 * @brief コンストラクタ
                 *
                 * コンストラクタ。
                 */
                Bg770aInterface(void)
                    : MainUartReceived_{nullptr},
                      MainUartReceived2_{nullptr},
                      RealMainUart_{BG770AINTERFACE_MAIN_UARTE, BG770AINTERFACE_MAIN_UARTE_IRQn, BG770AINTERFACE_MAIN_TXD, BG770AINTERFACE_MAIN_RXD, BG770AINTERFACE_MAIN_CTS, BG770AINTERFACE_MAIN_RTS},
                      MainUart_{RealMainUart_}
                {
                    MainUartReceived_ = xSemaphoreCreateBinary();  // FreeRTOS
                    MainUartReceived2_ = xSemaphoreCreateBinary(); // FreeRTOS
                }

                /**
                 * @~Japanese
                 * @brief MainUARTの割り込み処理
                 *
                 * MainUARTの割り込み処理です。
                 * UARTの割り込みハンドラから呼び出す必要があります。
                 * ```cpp
                 * extern "C" void BG770AINTERFACE_MAIN_UARTE_IRQHANDLER(void)
                 * {
                 *     WioCellularModuleInterfaceInstance.mainUartIrqHandler();
                 * }
                 * ```
                 */
                void mainUartIrqHandler(void)
                {
                    BaseType_t higherPriorityTaskWoken = pdFALSE; // FreeRTOS

                    RealMainUart_.IrqHandler();
                    if (MainUartReceived_)
                    {
                        xSemaphoreGiveFromISR(MainUartReceived_, &higherPriorityTaskWoken); // FreeRTOS
                    }
                    if (MainUartReceived2_)
                    {
                        xSemaphoreGiveFromISR(MainUartReceived2_, &higherPriorityTaskWoken); // FreeRTOS
                    }

                    portYIELD_FROM_ISR(higherPriorityTaskWoken); // FreeRTOS
                }

                /**
                 * @~Japanese
                 * @brief VDD_EXTの割り込み処理
                 *
                 * VDD_EXTの割り込み処理です。
                 * GPIOの変化割り込みから呼び出す必要があります。
                 * ```cpp
                 * attachInterrupt(BG770AINTERFACE_VDD_EXT, BG770AINTERFACE_VDD_EXT_IRQHANDLER, CHANGE);
                 *
                 * void BG770AINTERFACE_VDD_EXT_IRQHANDLER(void)
                 * {
                 *     WioCellularModuleInterfaceInstance.vddExtHandler();
                 * }
                 * ```
                 */
                void vddExtHandler(void)
                {
                    if (isActive())
                    {
                        MainDcd_.begin(INPUT);
                        MainRi_.begin(INPUT);
                        MainUart_.begin(115200);
                    }
                    else
                    {
                        MainDcd_.end();
                        MainRi_.end();
                        MainUart_.end();
                    }
                }

                /**
                 * @~Japanese
                 * @brief インターフェースを開始
                 *
                 * インターフェースを初期化します。
                 */
                void begin(void)
                {
                    VddExt_.begin(INPUT_PULLUP);
#if defined(BOARD_VERSION_ES2)
                    Pwrkey_.begin(OUTPUT, 0);
#elif defined(BOARD_VERSION_1_0)
                    Pwrkey_.begin(OUTPUT_S0D1, 1);
#else
#error "Unknown board version"
#endif
#ifdef BG770AINTERFACE_RESET_N
                    ResetN_.begin(OUTPUT_S0D1, 1);
#endif // BG770AINTERFACE_RESET_N
                    MainDtr_.begin(OUTPUT, 0);

                    if (isActive())
                    {
                        // Becomes active at startup for a certain period of time. Therefore, it waits until it becomes inactive.
                        const auto start = millis();
                        while (isActive())
                        {
                            if (millis() - start >= 250 + 2)
                            {
                                break;
                            }
                            delay(10);
                        }
                        if (isActive())
                        {
                            printf("---> Interface is active when begin()\n");
                            vddExtHandler();
                        }
                    }

                    attachInterrupt(BG770AINTERFACE_VDD_EXT, BG770AINTERFACE_VDD_EXT_IRQHANDLER, CHANGE);
                }

                /**
                 * @~Japanese
                 * @brief 電源をオン
                 *
                 * 電源をオンします。
                 */
                void powerOn(void)
                {
#if defined(BOARD_VERSION_ES2)
                    Pwrkey_.write(1);
                    delay(500 + 2);
                    Pwrkey_.write(0);
#elif defined(BOARD_VERSION_1_0)
                    Pwrkey_.write(0);
                    delay(500 + 2);
                    Pwrkey_.write(1);
#else
#error "Unknown board version"
#endif
                }

                /**
                 * @~Japanese
                 * @brief 電源をオフ
                 *
                 * 電源をオフします。
                 */
                void powerOff(void)
                {
#if defined(BOARD_VERSION_ES2)
                    Pwrkey_.write(1);
                    delay(650 + 2);
                    Pwrkey_.write(0);
#elif defined(BOARD_VERSION_1_0)
                    Pwrkey_.write(0);
                    delay(650 + 2);
                    Pwrkey_.write(1);
#else
#error "Unknown board version"
#endif
                }

                /**
                 * @~Japanese
                 * @brief リセット
                 *
                 * リセットします。
                 */
                void reset(void)
                {
#ifdef BG770AINTERFACE_RESET_N
                    ResetN_.write(0);
                    delay(100 + 2);
                    ResetN_.write(1);
#endif // BG770AINTERFACE_RESET_N
                }

                /**
                 * @~Japanese
                 * @brief 起動状態を取得
                 *
                 * @retval true 起動している
                 * @retval false 起動していない
                 *
                 * 起動状態を取得します。
                 */
                bool isActive(void)
                {
                    return !VddExt_.read();
                }

                /**
                 * @~Japanese
                 * @brief スリープ
                 *
                 * スリープモードもしくはeDRXモードへ遷移します。
                 */
                void sleep(void)
                {
                    MainDtr_.write(1);
                }

                /**
                 * @~Japanese
                 * @brief ウェイクアップ
                 *
                 * スリープモードから復帰します。
                 */
                void wakeup(void)
                {
                    MainDtr_.write(0);
                }

                /**
                 * @~Japanese
                 * @brief 受信通知セマフォを取得
                 *
                 * 受信通知セマフォを取得します。
                 */
                SemaphoreHandle_t getReceivedNotificationSemaphone(void) // FreeRTOS
                {
                    return MainUartReceived2_;
                }

                /**
                 * @~Japanese
                 * @brief 読み込み可能待ち
                 *
                 * @param [in] timeout タイムアウト時間[ミリ秒]。
                 *
                 * 読み込みが可能になるまで待ちます。
                 * この関数から返っても受信データがあることを保証するものではありません。
                 * timeout<0の場合は読み込み可能になるまで永久に待機します。
                 */
                void waitReadAvailable(int timeout)
                {
                    xSemaphoreTake(MainUartReceived_, timeout >= 0 ? pdMS_TO_TICKS(timeout) : portMAX_DELAY); // FreeRTOS
                }

                /**
                 * @~Japanese
                 * @brief 1バイト読み込み
                 *
                 * @retval <0 受信データ無し
                 * @retval >=0 受信データ
                 *
                 * MainUARTから受信したデータを読み込みます。
                 * 受信データが無いときは負の値を返します。
                 */
                int read(void)
                {
                    return MainUart_.read();
                }

                /**
                 * @~Japanese
                 * @brief 1バイト書き込み
                 *
                 * @param [in] data 送信データ。
                 *
                 * MainUARTへ送信するデータを書き込みます。
                 */
                void write(int data)
                {
                    MainUart_.write(data);
                }
            };

        }
    }
}

#endif // BG770AINTERFACE_HPP
