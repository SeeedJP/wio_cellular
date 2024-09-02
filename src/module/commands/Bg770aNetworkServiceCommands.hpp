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

template <typename MODULE>
class Bg770aNetworkServiceCommands
{
public:
    /**
     * @~Japanese
     * @brief ネットワークオペレーターを取得
     *
     * @param [out] mode モード
     * @param [out] format オペレーターの書式
     * @param [out] oper オペレーター名
     * @param [out] act アクセス技術
     * @return 実行結果
     *
     * ネットワークオペレーターを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     * * mode=-1: 無し
     * * mode=0: 自動
     * * mode=1: 手動
     * * mode=2: 登録解除
     * * mode=4: 手動->自動
     * * format=-1: 無し
     * * format=0: 長い書式
     * * format=1: 短い書式
     * * format=2: 数字
     * * act=-1: 無し
     * * act=7: eMTC
     * * act=9: NB-IoT
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

                    AtParameterParser parser{responseParameter};
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
     * @param [out] rssi 受信信号強度
     * @param [out] ber ビット誤り率
     * @return 実行結果
     *
     * 受信信号強度とビット誤り率を取得します。
     * * rssi=0   : -113[dBm]以下
     * * rssi=1   : -111[dBm]
     * * rssi=2~30: -109~-53[dBm]
     * * rssi=31  : -51[dBm]以上
     * * rssi=99  : 不明
     * * ber=0: 0~0.2%
     * * ber=0: 0.2~0.4%
     * * ber=0: 0.4~0.8%
     * * ber=0: 0.8~1.6%
     * * ber=0: 1.6~3.2%
     * * ber=0: 3.2~6.4%
     * * ber=0: 6.4~12.8%
     * * ber=0: 12.8~%
     * * ber=99 : 不明
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
                    AtParameterParser parser{responseParameter};
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
     * @param [in] mode モード
     * @param [in] actType アクセス技術タイプ
     * @param [in] edrxCycle eDRX周期
     * @return 実行結果
     *
     * eDRXを設定します。
     * * mode=0: 無効
     * * mode=1: 有効
     * * mode=2: 有効かつURC通知を有効
     * * mode=3: 無効かつ設定値をデフォルトに戻す
     * * actType=4: eMTC
     * * actType=5: NB-IoT
     * * edrxCycle=0: 5.12秒
     * * edrxCycle=1: 10.24秒
     * * edrxCycle=2: 20.48秒
     * * edrxCycle=3: 40.96秒
     * * edrxCycle=4: 61.44秒
     * * edrxCycle=5: 81.92秒
     * * edrxCycle=6: 102.4秒
     * * edrxCycle=7: 122.88秒
     * * edrxCycle=8: 143.36秒
     * * edrxCycle=9: 163.84秒
     * * edrxCycle=10: 327.68秒
     * * edrxCycle=11: 655.36秒
     * * edrxCycle=12: 1310.72秒
     * * edrxCycle=13: 2621.44秒
     * * edrxCycle=14: 5242.88秒
     * * edrxCycle=15: 10485.76秒
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
     * @param [out] phoneNumber 電話番号
     * @return 実行結果
     *
     * 電話番号を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * 例：07043466052
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
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 3) return false;
                    if (phoneNumber) *phoneNumber = parser[1];
                    return true;
                }
                return false; },
            300);
    }
};

#endif // BG770ANETWORKSERVICECOMMANDS_HPP
