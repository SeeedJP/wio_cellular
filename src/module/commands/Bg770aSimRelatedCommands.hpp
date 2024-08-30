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

template <typename MODULE>
class Bg770aSimRelatedCommands
{
public:
    /**
     * @~Japanese
     * @brief IMSIを取得
     *
     * @param [out] imsi IMSI(international mobile subscriber identity)
     * @return 実行結果
     *
     * IMSI(international mobile subscriber identity)を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * 例：440103167698583
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
     * @param [out] state PIN状態
     * @return 実行結果
     *
     * PIN状態を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     * * "READY": パスワードを要求しない
     * * "SIM PIN": SIM PINを要求
     * * "SIM PUK": SIM PUKを要求
     * * "SIM PIN2": SIM PIN2を要求
     * * "SIM PUK2": SIM PUK2を要求
     * * "PH-SIM PIN": 電話からSIMへのパスワード待ち
     * * "PH-NET PIN": ネットワークからSIMへのパスワード待ち
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
                    AtParameterParser parser{responseParameter};
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
     * @param [out] iccid ICCID(integrated circuit card identifier)
     * @return 実行結果
     *
     * ICCID(integrated circuit card identifier)を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * 例：8981100005810680869F
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
                    AtParameterParser parser{responseParameter};
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
     * @param [out] status SIM初期化ステータス
     * @return 実行結果
     *
     * SIMの初期化ステータスを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     * * 0: 初期状態
     * * 1: CPIN READY
     * * 2: SMS DONE
     * * 3: CPIN READY & SMS DONE
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
                    AtParameterParser parser{responseParameter};
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
     * @param [out] enable URC通知設定
     * @param [out] status SIM挿入ステータス
     * @return 実行結果
     *
     * SIM挿入のステータスとURC通知設定を取得します。
     * * enable=0: URC通知が無効
     * * enable=1: URC通知が有効
     * * status=0: SIM無し
     * * status=1: SIM有り
     * * status=2: SIM有無不明
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
                    AtParameterParser parser{responseParameter};
                    if (parser.size() != 2) return false;
                    if (enable) *enable = std::stoi(parser[0]);
                    if (status) *status = std::stoi(parser[1]);
                    return true;
                }
                return false; },
            300);
    }
};

#endif // BG770ASIMRELATEDCOMMANDS_HPP
