/*
 * Bg770aPacketDomainCommands.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770APACKETDOMAINCOMMANDS_HPP
#define BG770APACKETDOMAINCOMMANDS_HPP

#include <vector>
#include "module/at_client/AtParameterParser.hpp"
#include "internal/Misc.hpp"
#include "WioCellularResult.hpp"

template <typename MODULE>
class Bg770aPacketDomainCommands
{
public:
    /**
     * @~Japanese
     * @brief PDPコンテキスト
     */
    struct PdpContext
    {
        /**
         * @~Japanese
         * @brief PDPコンテキストID
         * 1~15
         */
        int cid;
        /**
         * @~Japanese
         * @brief PDPタイプ
         * * "IP": IPv4
         * * "PPP": PPP
         * * "IPV6": IPv6
         * * "IPV4V6": IPv4v6
         * * "Non-IP": Non-IP
         */
        std::string pdpType;
        /**
         * @~Japanese
         * @brief APN(access point name)
         */
        std::string apn;
        /**
         * @~Japanese
         * @brief PDPアドレス
         */
        std::string pdpAddr;
        /**
         * @~Japanese
         * @brief PDPデータ圧縮
         * * 0: OFF
         * * 1: ON
         * * 2: V.42bis
         */
        int dComp;
        /**
         * @~Japanese
         * @brief PDPヘッダ圧縮
         * * 0: OFF
         * * 1: ON
         * * 2: RFC 1144
         * * 3: RFC 2507
         * * 4: RFC 3095
         */
        int hComp;
        /**
         * @~Japanese
         * @brief IPv4アドレス割り当て
         * * 0: NASシグナリングによるIPv4アドレス割り当て
         */
        int ipV4AddrAlloc;
    };

    /**
     * @~Japanese
     * @brief PDPコンテキストステータス
     */
    struct PdpContextStatus
    {
        /**
         * @~Japanese
         * @brief PDPコンテキストID
         * 1~15
         */
        int cid;
        /**
         * @~Japanese
         * @brief 状態
         * * 0: 切断
         * * 1: 接続
         */
        int state;
    };

    /**
     * @~Japanese
     * @brief パケットドメインサービスの状態を取得
     *
     * @param [out] state 状態
     * @return 実行結果
     *
     * パケットドメインサービスの状態を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     * * state=-1: 無し
     * * state=0: 切断
     * * state=1: 接続
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 8.1. AT+CGATT PS Attach or Detach
     */
    WioCellularResult getPacketDomainState(int *state)
    {
        if (state)
            *state = -1;

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+CGATT?", [state](const std::string &response) -> bool
            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+CGATT: ", &responseParameter))
                {
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 1) return false;
                    if (state) *state = std::stoi(parser[0]);
                    return true;
                }
                return false; },
            140000);
    }

    /**
     * @~Japanese
     * @brief PDPコンテキストを設定
     *
     * @param [in] context PDPコンテキスト
     * @return 実行結果
     *
     * PDPコンテキストを設定します。
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 8.2. AT+CGDCONT Define PDP Context
     */
    WioCellularResult setPdpContext(const PdpContext &context)
    {
        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+CGDCONT=%d,\"%s\",\"%s\",\"%s\",%d,%d,%d", context.cid, context.pdpType.c_str(), context.apn.c_str(), context.pdpAddr.c_str(), context.dComp, context.hComp, context.ipV4AddrAlloc), 300);
    }

    /**
     * @~Japanese
     * @brief PDPコンテキストを取得
     *
     * @param [out] contexts PDPコンテキスト
     * @return 実行結果
     *
     * PDPコンテキストを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 8.2. AT+CGDCONT Define PDP Context
     */
    WioCellularResult getPdpContext(std::vector<PdpContext> *contexts)
    {
        if (contexts)
            contexts->clear();

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+CGDCONT?", [contexts](const std::string &response) -> bool
            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+CGDCONT: ", &responseParameter))
                {
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 7) return false;
                    if (contexts) contexts->push_back({std::stoi(parser[0]), parser[1], parser[2], parser[3], std::stoi(parser[4]), std::stoi(parser[5]), std::stoi(parser[6])});
                    return true;
                }
                return false; },
            300);
    }

    /**
     * @~Japanese
     * @brief PDPコンテキストステータスを取得
     *
     * @param [out] statuses PDPコンテキストステータス
     * @return 実行結果
     *
     * PDPコンテキストステータスを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 8.3. AT+CGACT PDP Context Activate or Deactivate
     */
    WioCellularResult getPdpContextStatus(std::vector<PdpContextStatus> *statuses)
    {
        if (statuses)
            statuses->clear();

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+CGACT?", [statuses](const std::string &response) -> bool
            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+CGACT: ", &responseParameter))
                {
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 2) return false;
                    if (statuses) statuses->push_back({std::stoi(parser[0]), std::stoi(parser[1])});
                    return true;
                }
                return false; },
            150000);
    }

    /**
     * @~Japanese
     * @brief EPSネットワーク登録ステータスのURC通知を設定
     *
     * @param [in] n URC通知設定
     * @return 実行結果
     *
     * EPSネットワーク登録ステータスのURC通知を設定します。
     * * n=0: URC通知を無効
     * * n=1: URC通知を有効 "+CEREG: <stat>"
     * * n=2: URC通知を有効 "+CEREG: <stat>[,[<tac>],[<ci>],[<AcT>]]"
     * * n=4: URC通知を有効 "+CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<Active-Time>],[<Periodic-TAU>]]]]"
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 8.8. AT+CEREG EPS Network Registration Status
     */
    WioCellularResult setEpsNetworkRegistrationStatusUrc(int n)
    {
        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+CEREG=%d", n), 300);
    }

    /**
     * @~Japanese
     * @brief EPSネットワーク登録状態を取得
     *
     * @param [out] state EPSネットワーク登録状態
     * @return 実行結果
     *
     * EPSネットワーク登録状態を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     * * state=-1: 無し
     * * state=0: 未登録
     * * state=1: 登録済み、ホームネットワーク
     * * state=2: 登録中
     * * state=3: 登録拒否
     * * state=4: 不明
     * * state=5: 登録済み、ローミング
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 8.8. AT+CEREG EPS Network Registration Status
     */
    WioCellularResult getEpsNetworkRegistrationState(int *state)
    {
        if (state)
            *state = -1;

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+CEREG?", [state](const std::string &response) -> bool
            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+CEREG: ", &responseParameter))
                {
                    AtParameterParser parser{responseParameter};
                    if (parser.size() < 2) return false;
                    if (state) *state = std::stoi(parser[1]);
                    return true;
                }
                return false; },
            300);
    }
};

#endif // BG770APACKETDOMAINCOMMANDS_HPP
