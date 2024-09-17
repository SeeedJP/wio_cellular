/*
 * Bg770aSimRelatedCommands.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770ASIMRELATEDCOMMANDS_HPP
#define BG770ASIMRELATEDCOMMANDS_HPP

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
                 * @brief Quectel BG770AモジュールのSIM関連コマンド
                 *
                 * @tparam MODULE モジュールのクラス
                 *
                 * Quectel BG770AモジュールのSIM関連コマンドです。
                 */
                template <typename MODULE>
                class Bg770aSimRelatedCommands
                {
                public:
                    /**
                     * @~Japanese
                     * @brief IMSIを取得
                     *
                     * @param [out] imsi IMSI(international mobile subscriber identity)。nullptrを指定すると値を代入しません。
                     * @return 実行結果。
                     *
                     * IMSI(international mobile subscriber identity)を取得します。
                     *
                     * 例: imsi = "440103167698583"
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 5.1. AT+CIMI Request International Mobile Subscriber Identity (IMSI)
                     */
                    WioCellularResult getIMSI(std::string *imsi)
                    {
                        if (imsi)
                            imsi->clear();

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+CIMI", [imsi](const std::string &response) -> bool
                            {
                                if (imsi) *imsi = response;
                                return true; },
                            300);
                    }

                    /**
                     * @~Japanese
                     * @brief PIN状態を取得
                     *
                     * @param [out] state PIN状態。nullptrを指定すると値を代入しません。
                     *   @arg "READY": パスワードを要求しない
                     *   @arg "SIM PIN": SIM PINを要求
                     *   @arg "SIM PUK": SIM PUKを要求
                     *   @arg "SIM PIN2": SIM PIN2を要求
                     *   @arg "SIM PUK2": SIM PUK2を要求
                     *   @arg "PH-SIM PIN": 電話からSIMへのパスワード待ち
                     *   @arg "PH-NET PIN": ネットワークからSIMへのパスワード待ち
                     * @return 実行結果。
                     *
                     * PIN状態を取得します。
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 5.3. AT+CPIN Enter PIN
                     */
                    WioCellularResult getSimState(std::string *state)
                    {
                        if (state)
                            state->clear();

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+CPIN?", [state](const std::string &response) -> bool
                            {
                                std::string responseParameter;
                                if (internal::stringStartsWith(response, "+CPIN: ", &responseParameter))
                                {
                                    at_client::AtParameterParser parser{responseParameter};
                                    if (parser.size() != 1) return false;
                                    if (state) *state = parser[0];
                                    return true;
                                }
                                return false; },
                            5000);
                    }

                    /**
                     * @~Japanese
                     * @brief ICCIDを取得
                     *
                     * @param [out] iccid ICCID(integrated circuit card identifier)。nullptrを指定すると値を代入しません。
                     * @return 実行結果。
                     *
                     * ICCID(integrated circuit card identifier)を取得します。
                     *
                     * 例: iccid = "8981100005810680869F"
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 5.6. AT+QCCID Show ICCID
                     */
                    WioCellularResult getSimCCID(std::string *iccid)
                    {
                        if (iccid)
                            iccid->clear();

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+QCCID", [iccid](const std::string &response) -> bool
                            {
                                std::string responseParameter;
                                if (internal::stringStartsWith(response, "+QCCID: ", &responseParameter))
                                {
                                    at_client::AtParameterParser parser{responseParameter};
                                    if (parser.size() != 1) return false;
                                    if (iccid) *iccid = parser[0];
                                    return true;
                                }
                                return false; },
                            300);
                    }

                    /**
                     * @~Japanese
                     * @brief SIM初期化ステータスを取得
                     *
                     * @param [out] status SIM初期化ステータス。nullptrを指定すると値を代入しません。
                     *   @arg 0: 初期状態
                     *   @arg 1: CPIN READY
                     *   @arg 2: SMS DONE
                     *   @arg 3: CPIN READY & SMS DONE
                     * @return 実行結果。
                     *
                     * SIMの初期化ステータスを取得します。
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 5.8. AT+QINISTAT Query Initialization Status of (U)SIM Card
                     */
                    WioCellularResult getSimInitializationStatus(int *status)
                    {
                        if (status)
                            *status = -1;

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+QINISTAT", [status](const std::string &response) -> bool
                            {
                                std::string responseParameter;
                                if (internal::stringStartsWith(response, "+QINISTAT: ", &responseParameter))
                                {
                                    at_client::AtParameterParser parser{responseParameter};
                                    if (parser.size() != 1) return false;
                                    if (status) *status = std::stoi(parser[0]);
                                    return true;
                                }
                                return false; },
                            300);
                    }

                    /**
                     * @~Japanese
                     * @brief SIM挿入ステータスを取得
                     *
                     * @param [out] enable URC通知設定。nullptrを指定すると値を代入しません。
                     *   @arg 0: URC通知が無効
                     *   @arg 1: URC通知が有効
                     * @param [out] status SIM挿入ステータス。nullptrを指定すると値を代入しません。
                     *   @arg 0: SIM無し
                     *   @arg 1: SIM有り
                     *   @arg 2: SIM有無不明
                     * @return 実行結果。
                     *
                     * SIM挿入のステータスとURC通知設定を取得します。
                     *
                     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
                     * > 5.10. AT+QSIMSTAT (U)SIM Card Insertion Status Report
                     */
                    WioCellularResult getSimInsertionStatus(int *enable, int *status)
                    {
                        if (enable)
                            *enable = -1;
                        if (status)
                            *status = -1;

                        return static_cast<MODULE &>(*this).queryCommand(
                            "AT+QSIMSTAT?", [enable, status](const std::string &response) -> bool
                            {
                                std::string responseParameter;
                                if (internal::stringStartsWith(response, "+QSIMSTAT: ", &responseParameter))
                                {
                                    at_client::AtParameterParser parser{responseParameter};
                                    if (parser.size() != 2) return false;
                                    if (enable) *enable = std::stoi(parser[0]);
                                    if (status) *status = std::stoi(parser[1]);
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

#endif // BG770ASIMRELATEDCOMMANDS_HPP
