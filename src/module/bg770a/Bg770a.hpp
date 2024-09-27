/*
 * Bg770a.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770A_HPP
#define BG770A_HPP

#include "commands/Bg770aExtendedConfigurationCommands.hpp"
#include "commands/Bg770aGeneralCommands.hpp"
#include "commands/Bg770aNetworkServiceCommands.hpp"
#include "commands/Bg770aPacketDomainCommands.hpp"
#include "commands/Bg770aSimRelatedCommands.hpp"
#include "commands/Bg770aTcpipCommands.hpp"

#include "module/at_client/AtClient.hpp"
#include "internal/Misc.hpp"
#include "WioCellularResult.hpp"

namespace wiocellular
{
    namespace module
    {
        namespace bg770a
        {

            /**
             * @~Japanese
             * @brief Quectel BG770Aモジュール
             *
             * @tparam INTERFACE インターフェースのクラス
             *
             * Quectel BG770Aモジュールのクラスです。
             */
            template <typename INTERFACE>
            class Bg770a : public at_client::AtClient<Bg770a<INTERFACE>>,
                           public commands::Bg770aExtendedConfigurationCommands<Bg770a<INTERFACE>>,
                           public commands::Bg770aGeneralCommands<Bg770a<INTERFACE>>,
                           public commands::Bg770aNetworkServiceCommands<Bg770a<INTERFACE>>,
                           public commands::Bg770aPacketDomainCommands<Bg770a<INTERFACE>>,
                           public commands::Bg770aSimRelatedCommands<Bg770a<INTERFACE>>,
                           public commands::Bg770aTcpipCommands<Bg770a<INTERFACE>>
            {
                friend class at_client::AtClient<Bg770a<INTERFACE>>;

            private:
                static constexpr int COMMAND_ECHO_TIMEOUT = 60000;

            private:
                INTERFACE &Interface_;

            public:
                /**
                 * @~Japanese
                 * @brief コンストラクタ
                 *
                 * @param [in] interface インターフェースのインスタンス。
                 *
                 * コンストラクタ。
                 * interfaceにインターフェースのインスタンスを指定します。
                 */
                explicit Bg770a(INTERFACE &interface) : at_client::AtClient<Bg770a<INTERFACE>>{},
                                                        Interface_{interface}
                {
                    at_client::AtClient<Bg770a<INTERFACE>>::registerUrcHandler([](const std::string &response) -> bool
                                                                               {
                                                                                    printf("URC> %s\n", response.c_str());
                                                                                    return false; });
                }

                /**
                 * @~Japanese
                 * @brief インターフェースを取得
                 *
                 * @return インターフェースのインスタンス。
                 *
                 * インターフェースを取得します。
                 */
                INTERFACE &getInterface(void)
                {
                    return Interface_;
                }

                /**
                 * @~Japanese
                 * @brief 実行コマンドを実行
                 *
                 * @param [in] command コマンド。
                 * @param [in] timeout タイムアウト時間[ミリ秒]。
                 * @return 実行結果。
                 *
                 * 実行コマンドを実行します。
                 */
                WioCellularResult executeCommand(const std::string &command, int timeout)
                {
                    printf("CMD> %s\n", command.c_str());
                    const auto start = millis();
                    if (!at_client::AtClient<Bg770a<INTERFACE>>::writeAndWaitCommand(command, COMMAND_ECHO_TIMEOUT))
                    {
                        return WioCellularResult::WaitCommandTimeout;
                    }
                    printf("ECO> %s ... %lu[ms]\n", command.c_str(), millis() - start);

                    std::string response;
                    while (true)
                    {
                        if ((response = at_client::AtClient<Bg770a<INTERFACE>>::readResponse(timeout)).empty())
                        {
                            return WioCellularResult::ReadResponseTimeout;
                        }

                        // Final Result Code
                        if (response == "OK")
                        {
                            printf("FRC> %s\n", response.c_str());
                            break;
                        }
                        if (response == "ERROR" || internal::stringStartsWith(response, "+CME ERROR: ") || internal::stringStartsWith(response, "+CMS ERROR: "))
                        {
                            printf("FRC> %s\n", response.c_str());
                            return WioCellularResult::CommandRejected;
                        }

                        // Unknown
                        printf("unk> %s\n", response.c_str());
                    }

                    return WioCellularResult::Ok;
                }

                /**
                 * @~Japanese
                 * @brief 問い合わせコマンドを実行
                 *
                 * @param [in] command コマンド。
                 * @param [in] informationTextHandler information textのハンドラ。
                 * @param [in] timeout タイムアウト時間[ミリ秒]。
                 * @return 実行結果。
                 *
                 * 問い合わせコマンドを実行します。
                 * informaton textを読み込んだときはinformationTextHandlerを呼び出します。
                 */
                WioCellularResult queryCommand(const std::string &command, const std::function<bool(const std::string &response)> &informationTextHandler, int timeout)
                {
                    printf("CMD> %s\n", command.c_str());
                    const auto start = millis();
                    if (!at_client::AtClient<Bg770a<INTERFACE>>::writeAndWaitCommand(command, COMMAND_ECHO_TIMEOUT))
                    {
                        return WioCellularResult::WaitCommandTimeout;
                    }
                    printf("ECO> %s ... %lu[ms]\n", command.c_str(), millis() - start);

                    std::string response;
                    while (true)
                    {
                        if ((response = at_client::AtClient<Bg770a<INTERFACE>>::readResponse(timeout)).empty())
                        {
                            return WioCellularResult::ReadResponseTimeout;
                        }

                        // Final Result Code
                        if (response == "OK")
                        {
                            printf("FRC> %s\n", response.c_str());
                            break;
                        }
                        if (response == "ERROR" || internal::stringStartsWith(response, "+CME ERROR: ") || internal::stringStartsWith(response, "+CMS ERROR: "))
                        {
                            printf("FRC> %s\n", response.c_str());
                            return WioCellularResult::CommandRejected;
                        }

                        // Information text
                        if (informationTextHandler && informationTextHandler(response))
                        {
                            printf("INF> %s\n", response.c_str());
                            continue;
                        }

                        // Unknown
                        printf("unk> %s\n", response.c_str());
                    }

                    return WioCellularResult::Ok;
                }

                /**
                 * @~Japanese
                 * @brief 送信コマンドを実行
                 *
                 * @param [in] command コマンド。
                 * @param [in] informationTextHandler information textのハンドラ。
                 * @param [in] timeout タイムアウト時間[ミリ秒]。
                 * @return 実行結果。
                 *
                 * 問い合わせコマンドを実行します。
                 * informaton textを読み込んだときはinformationTextHandlerを呼び出します。
                 */
                WioCellularResult sendCommand(const std::string &command, std::function<bool(const std::string &response)> informationTextHandler, int timeout)
                {
                    printf("CMD> %s\n", command.c_str());
                    const auto start = millis();
                    if (!at_client::AtClient<Bg770a<INTERFACE>>::writeAndWaitCommand(command, COMMAND_ECHO_TIMEOUT))
                    {
                        return WioCellularResult::WaitCommandTimeout;
                    }
                    printf("ECO> %s ... %lu[ms]\n", command.c_str(), millis() - start);

                    std::string response;
                    while (true)
                    {
                        if ((response = at_client::AtClient<Bg770a<INTERFACE>>::readResponse(timeout, [](const std::string &response) -> bool
                                                                                             { return response == "> "; }))
                                .empty())
                        {
                            return WioCellularResult::ReadResponseTimeout;
                        }

                        // Final Result Code
                        if (response == "SEND OK")
                        {
                            printf("FRC> %s\n", response.c_str());
                            break;
                        }
                        if (response == "ERROR" || response == "SEND FAIL")
                        {
                            printf("FRC> %s\n", response.c_str());
                            return WioCellularResult::CommandRejected;
                        }

                        // Information text
                        if (informationTextHandler && informationTextHandler(response))
                        {
                            printf("INF> %s\n", response.c_str());
                            continue;
                        }

                        // Unknown
                        printf("unk> %s\n", response.c_str());
                    }

                    return WioCellularResult::Ok;
                }

                /**
                 * @~Japanese
                 * @brief 電源をオン
                 *
                 * @param [in] timeout タイムアウト時間[ミリ秒]。
                 * @return 実行結果。
                 *
                 * 電源をオンします。
                 * 処理完了までに10秒程度かかります．
                 */
                WioCellularResult powerOn(int timeout)
                {
                    WioCellularResult result = WioCellularResult::Ok;

                    bool appRdy = false;
                    const auto handler = at_client::AtClient<Bg770a<INTERFACE>>::registerUrcHandler([&appRdy](const std::string &response) -> bool
                                                                                                    {
                                                                                                        if (response == "APP RDY")
                                                                                                        {
                                                                                                            appRdy = true;
                                                                                                            return true;
                                                                                                        }
                                                                                                        return false; });

                    if (!getInterface().isActive())
                    {
                        getInterface().powerOn();
                        if (!getInterface().isActive())
                        {
#if defined(BOARD_VERSION_ES2)
                            delay(2 + 2);
                            digitalWrite(PIN_VSYS_3V3_ENABLE, LOW);
                            delay(100 + 2);
                            digitalWrite(PIN_VSYS_3V3_ENABLE, HIGH);
                            delay(2 + 2);
                            getInterface().powerOn();
                            if (!getInterface().isActive())
                            {
                                printf("---> Interface is not active when powerOn()\n");
                                result = WioCellularResult::NotActivate;
                            }
#elif defined(BOARD_VERSION_1_0)
                            printf("---> Interface is not active when powerOn()\n");
                            result = WioCellularResult::NotActivate;
#else
#error "Unknown board version"
#endif
                        }
                    }
                    else
                    {
#if defined(BOARD_VERSION_ES2)
                        delay(2 + 2);
                        digitalWrite(PIN_VSYS_3V3_ENABLE, LOW);
                        delay(100 + 2);
                        digitalWrite(PIN_VSYS_3V3_ENABLE, HIGH);
                        delay(2 + 2);
                        getInterface().powerOn();
                        if (!getInterface().isActive())
                        {
                            printf("---> Interface is not active when powerOn()\n");
                            result = WioCellularResult::NotActivate;
                        }
#elif defined(BOARD_VERSION_1_0)
                        getInterface().reset();
#else
#error "Unknown board version"
#endif
                    }
                    if (result == WioCellularResult::Ok)
                    {
                        const auto start = millis();
                        while (!appRdy)
                        {
                            at_client::AtClient<Bg770a<INTERFACE>>::doWork(timeout - (millis() - start));
                            if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                            {
                                result = WioCellularResult::RdyTimeout;
                                break;
                            }
                        }
                    }

                    at_client::AtClient<Bg770a<INTERFACE>>::unregisterUrcHandler(handler);
                    if (result != WioCellularResult::Ok)
                    {
                        return result;
                    }

                    // Enable Hardware Flow Control
                    if ((result = executeCommand("AT+IFC=2,2", 300)) != WioCellularResult::Ok)
                    {
                        return result;
                    }

                    // Enable sleep mode
                    if ((result = executeCommand("AT+QSCLK=2", 300)) != WioCellularResult::Ok)
                    {
                        return result;
                    }

                    return result;
                }

                /**
                 * @~Japanese
                 * @brief 電源をオフ
                 *
                 * @return 実行結果。
                 *
                 * 電源をオフします。
                 */
                WioCellularResult powerOff(void)
                {
                    getInterface().powerOff();

                    return WioCellularResult::Ok;
                }
            };

        }
    }
}

#endif // BG770A_HPP
