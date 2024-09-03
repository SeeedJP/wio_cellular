/*
 * Bg770aNetworkServiceCommands.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770ANETWORKSERVICECOMMANDS_HPP
#define BG770ANETWORKSERVICECOMMANDS_HPP

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
                 * @brief Quectel BG770Aモジュールのネットワークサービスコマンド
                 *
                 * @tparam MODULE モジュールのクラス
                 *
                 * Quectel BG770Aモジュールのネットワークサービスコマンドです。
                 */
                template <typename MODULE>
                class Bg770aNetworkServiceCommands
                {
                public:
                    /**
                     * @~Japanese
                     * @brief ネットワークオペレーターを取得
                     *
                     * @param [out] mode モード。nullptrを指定すると値を代入しません。
                     *   @arg -1: 無し
                     *   @arg 0: 自動
                     *   @arg 1: 手動
                     *   @arg 2: 登録解除
                     *   @arg 4: 手動->自動
                     * @param [out] format オペレーターの書式。nullptrを指定すると値を代入しません。
                     *   @arg -1: 無し
                     *   @arg 0: 長い書式
                     *   @arg 1: 短い書式
                     *   @arg 2: 数字
                     * @param [out] oper オペレーター名。nullptrを指定すると値を代入しません。
                     * @param [out] act アクセステクノロジー。nullptrを指定すると値を代入しません。
                     *   @arg -1: 無し
                     *   @arg 7: eMTC
                     *   @arg 9: NB-IoT
                     * @return 実行結果。
                     *
                     * ネットワークオペレーターを取得します。
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 6.2. AT+COPS Operator Selection
                     */
                    WioCellularResult getOperator(int *mode, int *format, std::string *oper, int *act)
                    {
                        if (mode)
                            *mode = -1;
                        if (format)
                            *format = -1;
                        if (oper)
                            oper->clear();
                        if (act)
                            *act = -1;

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+COPS?", [mode, format, oper, act](const std::string &response) -> bool
                            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+COPS: ", &responseParameter))
                {
                    if (mode) *mode = -1;
                    if (format) *format = -1;
                    if (oper) oper->clear();
                    if (act) *act = -1;

                    at_client::AtParameterParser parser{responseParameter};
                    if (parser.size() >= 1 && mode) *mode = std::stoi(parser[0]);
                    if (parser.size() >= 2 && format) *format = std::stoi(parser[1]);
                    if (parser.size() >= 3 && oper) *oper = parser[2];
                    if (parser.size() >= 4 && act) *act = std::stoi(parser[3]);
                    return true;
                }
                return false; },
                            180000);
                    }

                    /**
                     * @~Japanese
                     * @brief 受信信号強度を取得
                     *
                     * @param [out] rssi 受信信号強度。nullptrを指定すると値を代入しません。
                     *   @arg 0: -113[dBm]以下
                     *   @arg 1: -111[dBm]
                     *   @arg 2~30: -109~-53[dBm]
                     *   @arg 31: -51[dBm]以上
                     *   @arg 99: 不明
                     * @param [out] ber ビット誤り率。nullptrを指定すると値を代入しません。
                     *   @arg 0: 0~0.2%
                     *   @arg 1: 0.2~0.4%
                     *   @arg 2: 0.4~0.8%
                     *   @arg 3: 0.8~1.6%
                     *   @arg 4: 1.6~3.2%
                     *   @arg 5: 3.2~6.4%
                     *   @arg 6: 6.4~12.8%
                     *   @arg 7: 12.8~%
                     *   @arg 99: 不明
                     * @return 実行結果。
                     *
                     * 受信信号強度とビット誤り率を取得します。
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 6.3. AT+CSQ Signal Quality
                     */
                    WioCellularResult getSignalQuality(int *rssi, int *ber)
                    {
                        if (rssi)
                            *rssi = -1;
                        if (ber)
                            *ber = -1;

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+CSQ", [rssi, ber](const std::string &response) -> bool
                            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+CSQ: ", &responseParameter))
                {
                    at_client::AtParameterParser parser{responseParameter};
                    if (parser.size() != 2) return false;
                    if (rssi) *rssi = std::stoi(parser[0]);
                    if (ber) *ber = std::stoi(parser[1]);
                    return true;
                }
                return false; },
                            300);
                    }

                    /**
                     * @~Japanese
                     * @brief eDRXを設定
                     *
                     * @param [in] mode モード。
                     *   @arg 0: 無効
                     *   @arg 1: 有効
                     *   @arg 2: 有効かつURC通知を有効
                     *   @arg 3: 無効かつ設定値をデフォルトに戻す
                     * @param [in] actType アクセステクノロジータイプ。
                     *   @arg 4: eMTC
                     *   @arg 5: NB-IoT
                     * @param [in] edrxCycle eDRX周期。
                     *   @arg 0: 5.12秒
                     *   @arg 1: 10.24秒
                     *   @arg 2: 20.48秒
                     *   @arg 3: 40.96秒
                     *   @arg 4: 61.44秒
                     *   @arg 5: 81.92秒
                     *   @arg 6: 102.4秒
                     *   @arg 7: 122.88秒
                     *   @arg 8: 143.36秒
                     *   @arg 9: 163.84秒
                     *   @arg 10: 327.68秒
                     *   @arg 11: 655.36秒
                     *   @arg 12: 1310.72秒
                     *   @arg 13: 2621.44秒
                     *   @arg 14: 5242.88秒
                     *   @arg 15: 10485.76秒
                     * @return 実行結果。
                     *
                     * eDRXを設定します。
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 6.10. AT+CEDRXS e-I-DRX Setting
                     */
                    WioCellularResult setEdrx(int mode, int actType, int edrxCycle)
                    {
                        assert(0 <= mode && mode <= 3);
                        assert(actType == 4 || actType == 5);
                        assert(0 <= edrxCycle && edrxCycle <= 15);

                        std::string edrxCycleStr;
                        for (int i = 0; i < 4; ++i)
                            edrxCycleStr += edrxCycle & 0b1000 >> i ? '1' : '0';

                        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+CEDRXS=%d,%d,\"%s\"", mode, actType, edrxCycleStr.c_str()), 300);
                    }

                    /**
                     * @~Japanese
                     * @brief 電話番号を取得
                     *
                     * @param [out] phoneNumber 電話番号。nullptrを指定すると値を代入しません。
                     * @return 実行結果。
                     *
                     * 電話番号を取得します。
                     *
                     * 例: phoneNumber = "07043466052"
                     *
                     * > EC25&EC21 AT Commands Manual @n
                     * > 8.1. AT+CNUM Subscriber Number
                     */
                    WioCellularResult getPhoneNumber(std::string *phoneNumber)
                    {
                        if (phoneNumber)
                            phoneNumber->clear();

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+CNUM", [phoneNumber](const std::string &response) -> bool
                            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+CNUM: ", &responseParameter))
                {
                    at_client::AtParameterParser parser{responseParameter};
                    if (parser.size() != 3) return false;
                    if (phoneNumber) *phoneNumber = parser[1];
                    return true;
                }
                return false; },
                            300);
                    }
                };

            }
        }
    }
}

#endif // BG770ANETWORKSERVICECOMMANDS_HPP
