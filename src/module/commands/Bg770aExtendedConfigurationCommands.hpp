/*
 * Bg770aExtendedConfigurationCommands.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770AEXTENDEDCONFIGURATIONCOMMANDS_HPP
#define BG770AEXTENDEDCONFIGURATIONCOMMANDS_HPP

#include "module/at_client/AtParameterParser.hpp"
#include "internal/Misc.hpp"
#include "WioCellularResult.hpp"

template <typename MODULE>
class Bg770aExtendedConfigurationCommands
{
public:
    /**
     * @~Japanese
     * @brief ネットワーク探索のアクセステクノロジー順序を取得
     *
     * @param [out] scanseq 探索するアクセステクノロジー順序
     * @return 実行結果
     *
     * ネットワーク探索のアクセステクノロジー順序を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     * * scanseq="0203": eMTC->NB-IoT
     * * scanseq="0302": NB-IoT->eMTC
     *
     * > BG77xA-GL&BG95xA-GL QCFG AT Commands Manual @n
     * > 2.1.1.3. AT+QCFG="nwscanseq" Configure RATs Searching Sequence
     */
    WioCellularResult getSearchAccessTechnologySequence(std::string *scanseq)
    {
        if (scanseq)
            scanseq->clear();

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+QCFG=\"nwscanseq\"", [scanseq](const std::string &response) -> bool
            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+QCFG: \"nwscanseq\",", &responseParameter))
                {
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 1) return false;
                    if (scanseq) *scanseq = parser[0];
                    return true;
                }
                return false; },
            300);
    }

    /**
     * @~Japanese
     * @brief ネットワーク探索のアクセステクノロジー順序を設定
     *
     * @param [in] scanseq 探索するアクセステクノロジー順序
     * @return 実行結果
     *
     * ネットワーク探索のアクセステクノロジー順序を設定します。
     * * scanseq="00": 自動(eMTC->NB-IoT)
     * * scanseq="02": eMTC->NB-IoT
     * * scanseq="0203": eMTC->NB-IoT
     * * scanseq="03": NB-IoT->eMTC
     * * scanseq="0302": NB-IoT->eMTC
     *
     * > BG77xA-GL&BG95xA-GL QCFG AT Commands Manual @n
     * > 2.1.1.3. AT+QCFG="nwscanseq" Configure RATs Searching Sequence
     */
    WioCellularResult setSearchAccessTechnologySequence(const std::string &scanseq)
    {
        assert(scanseq == "00" || scanseq == "02" || scanseq == "0203" || scanseq == "03" || scanseq == "0302");

        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+QCFG=\"nwscanseq\",%s", scanseq.c_str()), 300);
    }

    /**
     * @~Japanese
     * @brief ネットワーク探索の周波数バンドを取得
     *
     * @param [out] gsmBandValStr GSM周波数バンド
     * @param [out] emtcBandValStr eMTC周波数バンド
     * @param [out] nbiotBandValStr NB-IoT周波数バンド
     * @return 実行結果
     *
     * ネットワーク探索の周波数バンドを取得します。
     * * gsmBandValStr="0x0": 変更なし
     * * emtcBandValStr="0x0":                 変更なし
     * * emtcBandValStr="0x1":                 LTE B1
     * * emtcBandValStr="0x2":                 LTE B2
     * * emtcBandValStr="0x4":                 LTE B3
     * * emtcBandValStr="0x8":                 LTE B4
     * * emtcBandValStr="0x10":                LTE B5
     * * emtcBandValStr="0x80":                LTE B8
     * * emtcBandValStr="0x800":               LTE B12
     * * emtcBandValStr="0x1000":              LTE B13
     * * emtcBandValStr="0x20000":             LTE B18
     * * emtcBandValStr="0x40000":             LTE B19
     * * emtcBandValStr="0x80000":             LTE B20
     * * emtcBandValStr="0x1000000":           LTE B25
     * * emtcBandValStr="0x2000000":           LTE B26
     * * emtcBandValStr="0x4000000":           LTE B27
     * * emtcBandValStr="0x8000000":           LTE B28
     * * emtcBandValStr="0x20000000000000000": LTE B66
     * * nbiotBandValStr="0x0":                 変更なし
     * * nbiotBandValStr="0x1":                 LTE B1
     * * nbiotBandValStr="0x2":                 LTE B2
     * * nbiotBandValStr="0x4":                 LTE B3
     * * nbiotBandValStr="0x8":                 LTE B4
     * * nbiotBandValStr="0x10":                LTE B5
     * * nbiotBandValStr="0x80":                LTE B8
     * * nbiotBandValStr="0x800":               LTE B12
     * * nbiotBandValStr="0x1000":              LTE B13
     * * nbiotBandValStr="0x10000":             LTE B17
     * * nbiotBandValStr="0x20000":             LTE B18
     * * nbiotBandValStr="0x40000":             LTE B19
     * * nbiotBandValStr="0x80000":             LTE B20
     * * nbiotBandValStr="0x1000000":           LTE B25
     * * nbiotBandValStr="0x8000000":           LTE B28
     * * nbiotBandValStr="0x20000000000000000": LTE B66
     *
     * > BG77xA-GL&BG95xA-GL QCFG AT Commands Manual @n
     * > 2.1.1.4. AT+QCFG="band" Configure Frequency Band
     */
    WioCellularResult getSearchFrequencyBand(std::string *gsmBandValStr, std::string *emtcBandValStr, std::string *nbiotBandValStr)
    {
        if (gsmBandValStr)
            gsmBandValStr->clear();
        if (emtcBandValStr)
            emtcBandValStr->clear();
        if (nbiotBandValStr)
            nbiotBandValStr->clear();

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+QCFG=\"band\"", [gsmBandValStr, emtcBandValStr, nbiotBandValStr](const std::string &response) -> bool
            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+QCFG: \"band\",", &responseParameter))
                {
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 3) return false;
                    if (gsmBandValStr) *gsmBandValStr = parser[0];
                    if (emtcBandValStr) *emtcBandValStr = parser[1];
                    if (nbiotBandValStr) *nbiotBandValStr = parser[2];
                    return true;
                }
                return false; },
            300);
    }

    /**
     * @~Japanese
     * @brief ネットワーク探索の周波数バンドを設定
     *
     * @param [in] gsmBandValStr GSM周波数バンド
     * @param [in] emtcBandValStr eMTC周波数バンド
     * @param [in] nbiotBandValStr NB-IoT周波数バンド
     * @return 実行結果
     *
     * ネットワーク探索の周波数バンドを設定します。
     * * gsmBandValStr="0x0": 変更なし
     * * emtcBandValStr="0x0":                 変更なし
     * * emtcBandValStr="0x1":                 LTE B1
     * * emtcBandValStr="0x2":                 LTE B2
     * * emtcBandValStr="0x4":                 LTE B3
     * * emtcBandValStr="0x8":                 LTE B4
     * * emtcBandValStr="0x10":                LTE B5
     * * emtcBandValStr="0x80":                LTE B8
     * * emtcBandValStr="0x800":               LTE B12
     * * emtcBandValStr="0x1000":              LTE B13
     * * emtcBandValStr="0x20000":             LTE B18
     * * emtcBandValStr="0x40000":             LTE B19
     * * emtcBandValStr="0x80000":             LTE B20
     * * emtcBandValStr="0x1000000":           LTE B25
     * * emtcBandValStr="0x2000000":           LTE B26
     * * emtcBandValStr="0x4000000":           LTE B27
     * * emtcBandValStr="0x8000000":           LTE B28
     * * emtcBandValStr="0x20000000000000000": LTE B66
     * * nbiotBandValStr="0x0":                 変更なし
     * * nbiotBandValStr="0x1":                 LTE B1
     * * nbiotBandValStr="0x2":                 LTE B2
     * * nbiotBandValStr="0x4":                 LTE B3
     * * nbiotBandValStr="0x8":                 LTE B4
     * * nbiotBandValStr="0x10":                LTE B5
     * * nbiotBandValStr="0x80":                LTE B8
     * * nbiotBandValStr="0x800":               LTE B12
     * * nbiotBandValStr="0x1000":              LTE B13
     * * nbiotBandValStr="0x10000":             LTE B17
     * * nbiotBandValStr="0x20000":             LTE B18
     * * nbiotBandValStr="0x40000":             LTE B19
     * * nbiotBandValStr="0x80000":             LTE B20
     * * nbiotBandValStr="0x1000000":           LTE B25
     * * nbiotBandValStr="0x8000000":           LTE B28
     * * nbiotBandValStr="0x20000000000000000": LTE B66
     *
     * > BG77xA-GL&BG95xA-GL QCFG AT Commands Manual @n
     * > 2.1.1.4. AT+QCFG="band" Configure Frequency Band
     */
    WioCellularResult setSearchFrequencyBand(const std::string &gsmBandValStr, const std::string &emtcBandValStr, const std::string &nbiotBandValStr)
    {
        assert(!gsmBandValStr.empty());
        assert(!emtcBandValStr.empty());
        assert(!nbiotBandValStr.empty());

        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+QCFG=\"band\",%s,%s,%s", gsmBandValStr.c_str(), emtcBandValStr.c_str(), nbiotBandValStr.c_str()), 4500);
    }

    /**
     * @~Japanese
     * @brief ネットワーク探索のアクセステクノロジーを取得
     *
     * @param [out] mode 探索するアクセステクノロジー
     * @return 実行結果
     *
     * ネットワーク探索のアクセステクノロジーを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     * * mode=0: eMTC
     * * mode=1: NB-IoT
     * * mode=2: eMTCとNB-IoT
     *
     * > BG77xA-GL&BG95xA-GL QCFG AT Commands Manual @n
     * > 2.1.1.5. AT+QCFG="iotopmode" Configure Network Category to be Searched Under LTE RAT
     */
    WioCellularResult getSearchAccessTechnology(int *mode)
    {
        if (mode)
            *mode = -1;

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+QCFG=\"iotopmode\"", [mode](const std::string &response) -> bool
            {
                std::string responseParameter;
                if (internal::stringStartsWith(response, "+QCFG: \"iotopmode\",", &responseParameter))
                {
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 1) return false;
                    if (mode) *mode = std::stoi(parser[0]);
                    return true;
                }
                return false; },
            300);
    }

    /**
     * @~Japanese
     * @brief ネットワーク探索のアクセステクノロジーを設定
     *
     * @param [in] mode 探索するアクセステクノロジー
     * @return 実行結果
     *
     * ネットワーク探索のアクセステクノロジーを設定します。
     * * mode=0: eMTC
     * * mode=1: NB-IoT
     * * mode=2: eMTCとNB-IoT
     *
     * > BG77xA-GL&BG95xA-GL QCFG AT Commands Manual @n
     * > 2.1.1.5. AT+QCFG="iotopmode" Configure Network Category to be Searched Under LTE RAT
     */
    WioCellularResult setSearchAccessTechnology(int mode)
    {
        assert(0 <= mode && mode <= 2);

        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+QCFG=\"iotopmode\",%d", mode), 4500);
    }

    /**
     * @~Japanese
     * @brief PSM遷移のURC通知を設定
     *
     * @param [in] enable 有効
     * @return 実行結果
     *
     * PSM遷移のURC通知を設定します。
     *
     * > BG77xA-GL&BG95xA-GL QCFG AT Commands Manual @n
     * > 2.1.1.8. AT+QCFG="psm/urc" Enable/Disable PSM Entering Indication
     */
    WioCellularResult setPsmEnteringIndicationUrc(bool enable)
    {
        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+QCFG=\"psm/urc\",%d", enable ? 1 : 0), 300);
    }

    /**
     * @~Japanese
     * @brief PSMを設定
     *
     * @param [in] mode モード
     * @param [in] periodicTau 周期時間[秒]
     * @param [in] activeTau アクティブ時間[秒]
     * @return 実行結果
     *
     * PSMを設定します。
     * * mode=0: 無効
     * * mode=1: 有効
     *
     * > BG770A-GL&BG95xA-GL PSM Application Note @n
     * > 2.1.1.8. AT+QCFG="psm/urc" Enable/Disable PSM Entering Indication
     */
    WioCellularResult setPsm(int mode, int periodicTau, int activeTau)
    {
        assert(mode == 0 || mode == 1);
        assert(0 <= periodicTau && periodicTau / 36000 < 32);
        assert(0 <= activeTau && activeTau / 360 < 32);

        int periodic;
        if (periodicTau / 2 < 32) // 2sec
        {
            periodic = 0b011 << 5 | periodicTau / 2;
        }
        else if (periodicTau / 30 < 32) // 30sec
        {
            periodic = 0b100 << 5 | periodicTau / 30;
        }
        else if (periodicTau / 60 < 32) // 1min
        {
            periodic = 0b101 << 5 | periodicTau / 60;
        }
        else if (periodicTau / 600 < 32) // 10min
        {
            periodic = 0b000 << 5 | periodicTau / 600;
        }
        else if (periodicTau / 3600 < 32) // 1hour
        {
            periodic = 0b001 << 5 | periodicTau / 3600;
        }
        else if (periodicTau / 36000 < 32) // 10hour
        {
            periodic = 0b010 << 5 | periodicTau / 36000;
        }
        else
        {
            return WioCellularResult::ArgumentOutOfRange;
        }

        int active;
        if (activeTau / 2 < 32) // 2sec
        {
            active = 0b000 << 5 | activeTau / 2;
        }
        else if (activeTau / 60 < 32) // 1min
        {
            active = 0b001 << 5 | activeTau / 2;
        }
        else if (activeTau / 360 < 32) // 1/10hour
        {
            active = 0b010 << 5 | activeTau / 2;
        }
        else
        {
            return WioCellularResult::ArgumentOutOfRange;
        }

        std::string periodicStr;
        for (int i = 0; i < 8; ++i)
            periodicStr += periodic & 0b10000000 >> i ? '1' : '0';

        std::string activeStr;
        for (int i = 0; i < 8; ++i)
            activeStr += active & 0b10000000 >> i ? '1' : '0';

        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+CPSMS=%d,,,\"%s\",\"%s\"", mode, periodicStr.c_str(), activeStr.c_str()), 4000);
    }
};

#endif // BG770AEXTENDEDCONFIGURATIONCOMMANDS_HPP
