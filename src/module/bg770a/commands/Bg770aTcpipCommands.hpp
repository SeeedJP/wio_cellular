/*
 * Bg770aTcpipCommands.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770ATCPIPCOMMANDS_HPP
#define BG770ATCPIPCOMMANDS_HPP

#include <bitset>
#include <map>
#include <vector>
#include "module/at_client/AtParameterParser.hpp"
#include "internal/Misc.hpp"
#include "WioCellularResult.hpp"

namespace wiocellular
{
    namespace module
    {
        namespace bg770a
        {
            namespace commands
            {

                /**
                 * @~Japanese
                 * @brief Quectel BG770AモジュールのTCP/IPコマンド
                 *
                 * @tparam MODULE モジュールのクラス
                 *
                 * Quectel BG770AモジュールのTCP/IPコマンドです。
                 */
                template <typename MODULE>
                class Bg770aTcpipCommands
                {
                private:
                    static constexpr int COMMAND_ECHO_TIMEOUT = 10000;

                public:
                    /**
                     * @~Japanese
                     * @brief ソケットから受信する最大バイト数
                     */
                    static constexpr size_t RECEIVE_SOCKET_SIZE_MAX = 1500;

                private:
                    bool UrcSocketReceiveAttached_;
                    std::map<int, bool> UrcSocketReceiveNofity_;

                public:
                    /**
                     * @~Japanese
                     * @brief ソケットサービスステータス
                     */
                    struct SocketStatus
                    {
                        /**
                         * @~Japanese
                         * @brief 接続ID
                         * * 0~11
                         */
                        int connectId;
                        /**
                         * @~Japanese
                         * @brief サービスタイプ
                         * * "TCP": TCPクライアント
                         * * "UDP": UDPクライアント
                         * * "TCP LISTENER": TCPサーバーのリスナー
                         * * "TCP INCOMING": TCPサーバーの接続
                         * * "UDP SERVICE": UDP
                         */
                        std::string serviceType;
                        /**
                         * @~Japanese
                         * @brief IPアドレス
                         */
                        std::string ipAddress;
                        /**
                         * @~Japanese
                         * @brief リモートポート番号
                         * * 0~65535
                         */
                        int remotePort;
                        /**
                         * @~Japanese
                         * @brief ローカルポート番号
                         * * 0~65535
                         */
                        int localPort;
                        /**
                         * @~Japanese
                         * @brief ソケット状態
                         * * 0: 初期（未接続）
                         * * 1: 接続中
                         * * 2: 接続済み
                         * * 3: 接続待ち
                         * * 4: 切断中
                         */
                        int socketState;
                        /**
                         * @~Japanese
                         * @brief PDPコンテキストID
                         * * 1~5
                         */
                        int cid;
                        /**
                         * @~Japanese
                         * @brief サーバーの接続ID
                         * serviceTypeが"TCP INCOMING"のときのみ有効。
                         */
                        int serverId;
                        /**
                         * @~Japanese
                         * @brief データアクセスモード
                         * * 0: バッファアクセスモード
                         * * 1: ダイレクトプッシュモード
                         * * 2: 透過伝送モード
                         */
                        int accessMode;
                        /**
                         * @~Japanese
                         * @brief COMポート
                         * * "main": MAIN UART
                         * * "aux": AUX UART
                         * * "emux": EMUXモード
                         * * "usb": USB
                         */
                        std::string atPort;
                    };

                    /**
                     * @~Japanese
                     * @brief コンストラクタ
                     *
                     * コンストラクタ。
                     */
                    Bg770aTcpipCommands(void) : UrcSocketReceiveAttached_{false},
                                                UrcSocketReceiveNofity_{}
                    {
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットをオープン
                     *
                     * @param [in] cid PDPコンテキストID。
                     * @param [in] connectId 接続ID。
                     * @param [in] serviceType サービスタイプ。
                     * @param [in] ipAddress IPアドレス。
                     * @param [in] remotePort リモートポート番号。
                     * @param [in] localPort ローカルポート番号。
                     * @return 実行結果。
                     *
                     * ソケットをオープンします。
                     *
                     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
                     * > 2.3.5. AT+QIOPEN Open a Socket Service
                     */
                    WioCellularResult openSocket(int cid, int connectId, const std::string &serviceType, const std::string &ipAddress, int remotePort, int localPort)
                    {
                        assert(1 <= cid && cid <= 5);
                        assert(0 <= connectId && connectId <= 11);
                        assert(serviceType == "TCP" || serviceType == "UDP" || serviceType == "TCP LISTENER" || serviceType == "UDP SERVICE");
                        assert(!ipAddress.empty());
                        assert(0 <= remotePort && remotePort <= 65535);
                        assert(0 <= localPort && localPort <= 65535);

                        WioCellularResult result = WioCellularResult::Ok;

                        if (!UrcSocketReceiveAttached_)
                        {
                            static_cast<MODULE &>(*this).registerUrcHandler([this](const std::string &response) -> bool
                                                                            {
                                                                                std::string responseParameter;
                                                                                if (internal::stringStartsWith(response, "+QIURC: \"recv\",", &responseParameter))
                                                                                {
                                                                                    const auto connectId = std::stoi(responseParameter);
                                                                                    printf("---> Socket received (connectId=%d)\n", connectId);
                                                                                    auto nofity = UrcSocketReceiveNofity_.find(connectId);
                                                                                    if (nofity != UrcSocketReceiveNofity_.end())
                                                                                    {
                                                                                        nofity->second = true;
                                                                                    }
                                                                                    return true;
                                                                                }
                                                                                return false; });

                            UrcSocketReceiveAttached_ = true;
                        }
                        UrcSocketReceiveNofity_[connectId] = false;

                        bool opened = false;
                        int internalResult;
                        const auto handler = static_cast<MODULE &>(*this).registerUrcHandler([connectId, &opened, &internalResult](const std::string &response) -> bool
                                                                                             {
                                                                                                const std::string prefix = internal::stringFormat("+QIOPEN: %d,", connectId);
                                                                                                if (response.starts_with(prefix))
                                                                                                {
                                                                                                    opened = true;
                                                                                                    internalResult = std::stoi(response.substr(prefix.size()));
                                                                                                    return true;
                                                                                                }
                                                                                                return false; });

                        if ((result = static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+QIOPEN=%d,%d,\"%s\",\"%s\",%d,%d", cid, connectId, serviceType.c_str(), ipAddress.c_str(), remotePort, localPort), 300)) == WioCellularResult::Ok)
                        {
                            constexpr int timeout = 150000;
                            const auto start = millis();
                            while (!opened)
                            {
                                static_cast<MODULE &>(*this).doWork(timeout - (millis() - start));
                                if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                                {
                                    result = WioCellularResult::OpenTimeout;
                                    break;
                                }
                            }
                        }
                        static_cast<MODULE &>(*this).unregisterUrcHandler(handler);
                        if (result != WioCellularResult::Ok)
                        {
                            return result;
                        }
                        if (internalResult != 0)
                        {
                            return WioCellularResult::OpenError;
                        }

                        return result;
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットをクローズ
                     *
                     * @param [in] connectId 接続ID。
                     * @return 実行結果。
                     *
                     * ソケットをクローズします。
                     *
                     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
                     * > 2.3.6. AT+QICLOSE Close a Socket Service
                     */
                    WioCellularResult closeSocket(int connectId)
                    {
                        assert(0 <= connectId && connectId <= 11);

                        WioCellularResult result = WioCellularResult::Ok;

                        if ((result = static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+QICLOSE=%d", connectId), 11000)) != WioCellularResult::Ok)
                        {
                            return result;
                        }

                        UrcSocketReceiveNofity_.erase(connectId);

                        return result;
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットサービスステータスを取得
                     *
                     * @param [in] cid PDPコンテキストID。
                     * @param [out] statuses ソケットサービスステータス。nullptrを指定すると値を代入しません。
                     * @return 実行結果。
                     *
                     * ソケットサービスステータスを取得します。
                     *
                     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
                     * > 2.3.7. AT+QISTATE Query Socket Service Status
                     */
                    WioCellularResult getSocketStatus(int cid, std::vector<SocketStatus> *statuses)
                    {
                        assert(1 <= cid && cid <= 5);

                        if (statuses)
                            statuses->clear();

                        return static_cast<MODULE &>(*this).queryCommand(
                            internal::stringFormat("AT+QISTATE=0,%d", cid), [statuses](const std::string &response) -> bool
                            {
                                std::string responseParameter;
                                if (internal::stringStartsWith(response, "+QISTATE: ", &responseParameter))
                                {
                                    at_client::AtParameterParser parser{responseParameter};
                                    if (parser.size() != 10) return false;
                                    if (statuses) statuses->push_back({std::stoi(parser[0]), parser[1], parser[2], std::stoi(parser[3]), std::stoi(parser[4]), std::stoi(parser[5]), std::stoi(parser[6]), std::stoi(parser[7]), std::stoi(parser[8]), parser[9]});
                                    return true;
                                }
                                return false; },
                            300);
                    }

                    /**
                     * @~Japanese
                     * @brief 未使用の接続IDを取得
                     *
                     * @param [in] cid PDPコンテキストID。
                     * @param [out] unusedConnectId 未使用の接続ID。未使用が無い場合は-1を返します。
                     * @return 実行結果。
                     *
                     * 未使用の接続IDを取得します。
                     *
                     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
                     * > 2.3.7. AT+QISTATE Query Socket Service Status
                     */
                    WioCellularResult getSocketUnusedConnectId(int cid, int *unusedConnectId)
                    {
                        assert(1 <= cid && cid <= 5);
                        assert(unusedConnectId);

                        *unusedConnectId = -1;

                        WioCellularResult result;

                        std::bitset<12> usedConnectIds;
                        if ((result = static_cast<MODULE &>(*this).queryCommand(
                                 internal::stringFormat("AT+QISTATE=0,%d", cid), [&usedConnectIds](const std::string &response) -> bool
                                 {
                                    std::string responseParameter;
                                    if (internal::stringStartsWith(response, "+QISTATE: ", &responseParameter))
                                    {
                                        at_client::AtParameterParser parser{responseParameter};
                                        if (parser.size() != 10) return false;
                                        usedConnectIds[std::stoi(parser[0])] = true;
                                        return true;
                                    }
                                    return false; },
                                 300)) != WioCellularResult::Ok)
                        {
                            return result;
                        }

                        for (size_t i = 0; i < usedConnectIds.size(); ++i)
                        {
                            if (!usedConnectIds.test(i))
                            {
                                *unusedConnectId = i;
                                break;
                            }
                        }

                        return WioCellularResult::Ok;
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットへ送信
                     *
                     * @param [in] connectId 接続ID。
                     * @param [in] data データ。nullptrを指定すると送信しません。
                     * @param [in] dataSize データサイズ。0を指定すると送信しません。
                     * @return 実行結果。
                     *
                     * ソケットへ送信します。
                     *
                     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
                     * > 2.3.8. AT+QISEND Send Data
                     */
                    WioCellularResult sendSocket(int connectId, const void *data, size_t dataSize)
                    {
                        assert(0 <= connectId && connectId <= 11);

                        if (!data || dataSize <= 0)
                        {
                            return WioCellularResult::Ok;
                        }

                        return static_cast<MODULE &>(*this).sendCommand(
                            internal::stringFormat("AT+QISEND=%d,%d", connectId, dataSize), [this, data, dataSize](const std::string &response) -> bool
                            {
                                if (response == "> ")
                                {
                                    static_cast<MODULE &>(*this).writeBinary(data, dataSize);
                                    static_cast<MODULE &>(*this).readBinaryDiscard(dataSize, COMMAND_ECHO_TIMEOUT);
                                    return true;
                                }
                                return false; },
                            120000);
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットへ送信
                     *
                     * @param [in] connectId 接続ID。
                     * @param [in] data データ。長さ0の場合は送信しません。
                     * @return 実行結果。
                     *
                     * ソケットへ送信します。
                     */
                    WioCellularResult sendSocket(int connectId, const std::string &data)
                    {
                        return sendSocket(connectId, data.data(), data.size());
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットから未読のデータサイズを取得
                     *
                     * @param [in] connectId 接続ID。
                     * @param [out] availableSize 未読のデータサイズ。nullptrを指定すると値を代入しません。
                     * @return 実行結果。
                     *
                     * ソケットから未読のデータサイズを取得します。
                     *
                     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
                     * > 2.3.9. AT+QIRD Retrieve the Received TCP/IP Data
                     */
                    WioCellularResult getSocketReceiveAvailable(int connectId, size_t *availableSize)
                    {
                        assert(0 <= connectId && connectId <= 11);

                        if (availableSize)
                            *availableSize = -1;

                        return static_cast<MODULE &>(*this).queryCommand(
                            internal::stringFormat("AT+QIRD=%d,0", connectId), [availableSize](const std::string &response) -> bool
                            {
                                std::string responseParameter;
                                if (internal::stringStartsWith(response, "+QIRD: ", &responseParameter))
                                {
                                    at_client::AtParameterParser parser{responseParameter};
                                    if (parser.size() < 3) return false;
                                    if (availableSize) *availableSize = std::stoi(parser[2]);
                                    return true;
                                }
                                return false; },
                            120000);
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットから受信
                     *
                     * @param [in] connectId 接続ID。
                     * @param [in,out] data データ。nullptrを指定すると読み捨てます。
                     * @param [in] dataSize データサイズ。0を指定すると受信しません。
                     * @param [out] readDataSize 受信したデータサイズ。nullptrを指定すると値を代入しません。
                     * @return 実行結果。
                     *
                     * ソケットから受信します。
                     * 受信したデータが無いときは*readDataSize=0を返します。
                     * 値を得る必要が無いときはnullptrを指定できます。
                     *
                     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
                     * > 2.3.9. AT+QIRD Retrieve the Received TCP/IP Data
                     */
                    WioCellularResult receiveSocket(int connectId, void *data, size_t dataSize, size_t *readDataSize)
                    {
                        assert(0 <= connectId && connectId <= 11);

                        if (dataSize <= 0)
                        {
                            return WioCellularResult::Ok;
                        }
                        if (readDataSize)
                            *readDataSize = 0;

                        UrcSocketReceiveNofity_[connectId] = false;

                        return static_cast<MODULE &>(*this).queryCommand(
                            internal::stringFormat("AT+QIRD=%d,%d", connectId, dataSize), [this, data, dataSize, readDataSize](const std::string &response) -> bool
                            {
                                std::string responseParameter;
                                if (internal::stringStartsWith(response, "+QIRD: ", &responseParameter))
                                {
                                    at_client::AtParameterParser parser{responseParameter};
                                    if (parser.size() < 1) return false;
                                    const size_t actualDataSize = std::stoi(parser[0]);
                                    assert(actualDataSize <= dataSize);
                                    if (actualDataSize >= 1)
                                    {
                                        if (!static_cast<MODULE &>(*this).readBinary(data, actualDataSize, 120000))
                                        {
                                            return false;
                                        }
                                    }
                                    if (readDataSize) *readDataSize = actualDataSize;
                                    return true;
                                }
                                return false; },
                            120000);
                    }

                    /**
                     * @~Japanese
                     * @brief ソケットから受信
                     *
                     * @param [in] connectId 接続ID。
                     * @param [in,out] data データ。nullptrを指定すると読み捨てます。
                     * @param [in] dataSize データサイズ。0を指定すると受信しません。
                     * @param [out] readDataSize 受信したデータサイズ。nullptrを指定すると値を代入しません。
                     * @param [in] timeout タイムアウト時間[ミリ秒]。
                     * @return 実行結果。
                     *
                     * ソケットから受信します。
                     */
                    WioCellularResult receiveSocket(int connectId, void *data, size_t dataSize, size_t *readDataSize, int timeout)
                    {
                        WioCellularResult result = WioCellularResult::Ok;

                        const auto start = millis();
                        while (true)
                        {
                            if ((result = receiveSocket(connectId, data, dataSize, readDataSize)) != WioCellularResult::Ok)
                            {
                                return result;
                            }
                            if (*readDataSize >= 1)
                            {
                                return WioCellularResult::Ok;
                            }

                            do
                            {
                                static_cast<MODULE &>(*this).doWork(timeout - (millis() - start));
                                if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                                {
                                    return WioCellularResult::ReceiveTimeout;
                                }
                            } while (!UrcSocketReceiveNofity_[connectId]);
                        }
                    }
                };

            }
        }
    }
}

#endif // BG770ATCPIPCOMMANDS_HPP
