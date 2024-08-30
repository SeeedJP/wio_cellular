/*
 * Bg770aGeneralCommands.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770AGENERALCOMMANDS_HPP
#define BG770AGENERALCOMMANDS_HPP

#include "internal/Misc.hpp"
#include "WioCellularResult.hpp"

template <typename MODULE>
class Bg770aGeneralCommands
{
public:
    /**
     * @~Japanese
     * @brief IMEIを取得
     *
     * @param [out] imei IMEI(international mobile equipment identity)
     * @return 実行結果
     *
     * IMEI(international mobile equipment identity)を取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * 例：865502060000048
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 2.8. AT+GSN Request International Mobile Equipment Identity (IMEI)
     */
    WioCellularResult getIMEI(std::string *imei)
    {
        if (imei)
            imei->clear();

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+GSN", [imei](const std::string &response) -> bool
            {
                if (imei) *imei = response;
                return true; },
            300);
    }

    /**
     * @~Japanese
     * @brief 全ての設定をリセット
     *
     * @param [in] timeout タイムアウト時間[ミリ秒]
     * @return 実行結果
     *
     * 全ての設定を工場出荷時のデフォルトにします。
     * 処理完了までに20秒程度かかります．
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 2.10. AT&F Reset All AT Command Settings to Factory Settings
     */
    WioCellularResult factoryDefault(int timeout)
    {
        WioCellularResult result = WioCellularResult::Ok;

        bool appRdy = false;
        const auto handler = static_cast<MODULE &>(*this).registerUrcHandler([&appRdy](const std::string &response) -> bool
                                                                             {
            if (response == "APP RDY")
            {
                appRdy = true;
                return true;
            }
            return false; });

        if ((result = static_cast<MODULE &>(*this).executeCommand("AT&F1", 300)) == WioCellularResult::Ok)
        {
            const auto start = millis();
            while (!appRdy)
            {
                static_cast<MODULE &>(*this).doWork(timeout - (millis() - start));
                if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                {
                    result = WioCellularResult::RdyTimeout;
                    break;
                }
            }
        }
        static_cast<MODULE &>(*this).unregisterUrcHandler(handler);
        if (result != WioCellularResult::Ok)
        {
            return result;
        }

        // Enable Hardware Flow Control
        if ((result = static_cast<MODULE &>(*this).executeCommand("AT+IFC=2,2", 300)) != WioCellularResult::Ok)
        {
            return result;
        }

        return result;
    }

    /**
     * @~Japanese
     * @brief 電話の機能レベルを設定
     *
     * @param [in] fun 機能レベル
     * @return 実行結果
     *
     * 電話の機能レベルを設定します。
     * * fun=0: 最小機能
     * * fun=1: 全機能（RFフロントエンドとSIMカードを有効）
     * * fun=4: SIMカードのみ有効（RFフロントエンドは無効）
     *
     * > BG77xA-GL&BG95xA-GL AT Commands Manual @n
     * > 2.21. AT+CFUN Set UE Functionality
     */
    WioCellularResult setPhoneFunctionality(int fun)
    {
        return static_cast<MODULE &>(*this).executeCommand(internal::stringFormat("AT+CFUN=%d", fun), 300);
    }

    /**
     * @~Japanese
     * @brief ファームウェアのレビジョンを取得
     *
     * @param [out] revision ファームウェアのレビジョン
     * @return 実行結果
     *
     * ファームウェアのレビジョンを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * 例：BG770AGLAAR02A05_JP_01.200.01.200
     *
     * > BG96 AT Commands Manual @n
     * > 2.26. AT+QGMR Request Modem and Application Firmware Versions
     */
    WioCellularResult getModemInfo(std::string *revision)
    {
        if (revision)
            revision->clear();

        return static_cast<MODULE &>(*this).queryCommand(
            "AT+QGMR", [revision](const std::string &response) -> bool
            {
                if (revision) *revision = response;
                return true; },
            300);
    }
};

#endif // BG770AGENERALCOMMANDS_HPP
