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
                 * @brief Quectel BG770Aモジュールの拡張設定コマンド
                 *
                 * @tparam MODULE モジュールのクラス
                 *
                 * Quectel BG770Aモジュールの拡張設定コマンドです。
                 */
                template <typename MODULE>
                class Bg770aExtendedConfigurationCommands
                {
                public:
                    /**
                     * @~Japanese
                     * @brief ネットワーク探索のアクセステクノロジー順序を取得
                     *
                     * @param [out] scanseq 探索するアクセステクノロジー順序。nullptrを指定すると値を代入しません。
                     *   @arg "0203": eMTC->NB-IoT
                     *   @arg "0302": NB-IoT->eMTC
                     * @return 実行結果。
                     *
                     * ネットワーク探索のアクセステクノロジー順序を取得します。
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
                    at_client::AtParameterParser parser{responseParameter};
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
                     * @param [in] scanseq 探索するアクセステクノロジー順序。
                     *   @arg "00": 自動(eMTC->NB-IoT)
                     *   @arg "02": eMTC->NB-IoT
                     *   @arg "0203": eMTC->NB-IoT
                     *   @arg "03": NB-IoT->eMTC
                     *   @arg "0302": NB-IoT->eMTC
                     * @return 実行結果。
                     *
                     * ネットワーク探索のアクセステクノロジー順序を設定します。
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
                     * @param [out] gsmBandValStr GSM周波数バンド。nullptrを指定すると値を代入しません。
                     *   @arg "0x0": 変更なし
                     * @param [out] emtcBandValStr eMTC周波数バンド。nullptrを指定すると値を代入しません。
                     *   @arg "0x0": 変更なし
                     *   @arg "0x1": LTE B1
                     *   @arg "0x2": LTE B2
                     *   @arg "0x4": LTE B3
                     *   @arg "0x8": LTE B4
                     *   @arg "0x10": LTE B5
                     *   @arg "0x80": LTE B8
                     *   @arg "0x800": LTE B12
                     *   @arg "0x1000": LTE B13
                     *   @arg "0x20000": LTE B18
                     *   @arg "0x40000": LTE B19
                     *   @arg "0x80000": LTE B20
                     *   @arg "0x1000000": LTE B25
                     *   @arg "0x2000000": LTE B26
                     *   @arg "0x4000000": LTE B27
                     *   @arg "0x8000000": LTE B28
                     *   @arg "0x20000000000000000": LTE B66
                     * @param [out] nbiotBandValStr NB-IoT周波数バンド。nullptrを指定すると値を代入しません。
                     *   @arg "0x0": 変更なし
                     *   @arg "0x1": LTE B1
                     *   @arg "0x2": LTE B2
                     *   @arg "0x4": LTE B3
                     *   @arg "0x8": LTE B4
                     *   @arg "0x10": LTE B5
                     *   @arg "0x80": LTE B8
                     *   @arg "0x800": LTE B12
                     *   @arg "0x1000": LTE B13
                     *   @arg "0x10000": LTE B17
                     *   @arg "0x20000": LTE B18
                     *   @arg "0x40000": LTE B19
                     *   @arg "0x80000": LTE B20
                     *   @arg "0x1000000": LTE B25
                     *   @arg "0x8000000": LTE B28
                     *   @arg "0x20000000000000000": LTE B66
                     * @return 実行結果。
                     *
                     * ネットワーク探索の周波数バンドを取得します。
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
                    at_client::AtParameterParser parser{responseParameter};
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
                     * @param [in] gsmBandValStr GSM周波数バンド。
                     *   @arg "0x0": 変更なし
                     * @param [in] emtcBandValStr eMTC周波数バンド。
                     *   @arg "0x0": 変更なし
                     *   @arg "0x1": LTE B1
                     *   @arg "0x2": LTE B2
                     *   @arg "0x4": LTE B3
                     *   @arg "0x8": LTE B4
                     *   @arg "0x10": LTE B5
                     *   @arg "0x80": LTE B8
                     *   @arg "0x800": LTE B12
                     *   @arg "0x1000": LTE B13
                     *   @arg "0x20000": LTE B18
                     *   @arg "0x40000": LTE B19
                     *   @arg "0x80000": LTE B20
                     *   @arg "0x1000000": LTE B25
                     *   @arg "0x2000000": LTE B26
                     *   @arg "0x4000000": LTE B27
                     *   @arg "0x8000000": LTE B28
                     *   @arg "0x20000000000000000": LTE B66
                     * @param [in] nbiotBandValStr NB-IoT周波数バンド。
                     *   @arg "0x0": 変更なし
                     *   @arg "0x1": LTE B1
                     *   @arg "0x2": LTE B2
                     *   @arg "0x4": LTE B3
                     *   @arg "0x8": LTE B4
                     *   @arg "0x10": LTE B5
                     *   @arg "0x80": LTE B8
                     *   @arg "0x800": LTE B12
                     *   @arg "0x1000": LTE B13
                     *   @arg "0x10000": LTE B17
                     *   @arg "0x20000": LTE B18
                     *   @arg "0x40000": LTE B19
                     *   @arg "0x80000": LTE B20
                     *   @arg "0x1000000": LTE B25
                     *   @arg "0x8000000": LTE B28
                     *   @arg "0x20000000000000000": LTE B66
                     * @return 実行結果。
                     *
                     * ネットワーク探索の周波数バンドを設定します。
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
                     * @param [out] mode 探索するアクセステクノロジー。nullptrを指定すると値を代入しません。
                     *   @arg 0: eMTC
                     *   @arg 1: NB-IoT
                     *   @arg 2: eMTCとNB-IoT
                     * @return 実行結果。
                     *
                     * ネットワーク探索のアクセステクノロジーを取得します。
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
                    at_client::AtParameterParser parser{responseParameter};
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
                     * @param [in] mode 探索するアクセステクノロジー。
                     *   @arg 0: eMTC
                     *   @arg 1: NB-IoT
                     *   @arg 2: eMTCとNB-IoT
                     * @return 実行結果。
                     *
                     * ネットワーク探索のアクセステクノロジーを設定します。
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
                     * @param [in] enable 有効。
                     * @return 実行結果。
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
                     * @param [in] mode モード。
                     *   @arg 0: 無効
                     *   @arg 1: 有効
                     * @param [in] periodicTau 周期時間[秒]。
                     *   @arg 2~63: 2~63[秒](2秒単位)
                     *   @arg 64~959: 64~959[秒](30秒単位)
                     *   @arg 960~1919: 960~1919[秒](1分単位)
                     *   @arg 1920~19199: 1920~19199[秒](10分単位)
                     *   @arg 19200~115199: 19200~115199[秒](1時間単位)
                     *   @arg 115200~1151999: 115200~1151999[秒](10時間単位)
                     * @param [in] activeTau アクティブ時間[秒]。
                     *   @arg 2~63: 2~63[秒](2秒単位)
                     *   @arg 64~959: 64~1919[秒](1分単位)
                     *   @arg 1920~11519: 960~11519[秒](1/10時間単位)
                     * @return 実行結果。
                     *
                     * PSMを設定します。
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

            }
        }
    }
}

#endif // BG770AEXTENDEDCONFIGURATIONCOMMANDS_HPP
