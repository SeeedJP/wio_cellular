/*
 * Bg770a.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef BG770A_HPP
#define BG770A_HPP

#include <map>
#include <vector>
#include "at_client/AtClient.hpp"
#include "at_client/AtParameterParser.hpp"
#include "internal/Misc.hpp"
#include "../WioCellularResult.hpp"

/**
 * @~Japanese
 * @brief Quectel BG770Aモジュール
 *
 * @tparam INTERFACE インターフェースのクラス
 *
 * Quectel BG770Aモジュールのクラスです。
 */
template <typename INTERFACE>
class Bg770a : public AtClient<Bg770a<INTERFACE>> // CRTP
{
    friend class AtClient<Bg770a<INTERFACE>>;

private:
    static constexpr int COMMAND_ECHO_TIMEOUT = 10000;

private:
    INTERFACE &Interface_;
    bool UrcSocketReceiveAttached_;
    std::map<int, bool> UrcSocketReceiveNofity_;

public:
    /**
     * @~Japanese
     * @brief コンストラクタ
     *
     * @param [in] interface インターフェースのインスタンス
     *
     * コンストラクタ。
     * interfaceにインターフェースのインスタンスを指定します。
     */
    explicit Bg770a(INTERFACE &interface) : AtClient<Bg770a<INTERFACE>>{},
                                            Interface_{interface},
                                            UrcSocketReceiveAttached_{false},
                                            UrcSocketReceiveNofity_{}
    {
        AtClient<Bg770a<INTERFACE>>::registerUrcHandler([this](const std::string &response) -> bool
                                                        {
            printf("URC> %s\n", response.c_str());
            return false; });
    }

    /**
     * @~Japanese
     * @brief インターフェースを取得
     *
     * @return インターフェースのインスタンス
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
     * @param [in] command コマンド
     * @param [in] timeout タイムアウト時間[ミリ秒]
     * @return 実行結果
     *
     * 実行コマンドを実行します。
     */
    WioCellularResult executeCommand(const std::string &command, int timeout)
    {
        printf("CMD> %s\n", command.c_str());
        const auto start = millis();
        if (!AtClient<Bg770a<INTERFACE>>::writeAndWaitCommand(command, COMMAND_ECHO_TIMEOUT))
        {
            return WioCellularResult::WaitCommandTimeout;
        }
        printf("ECO> %s ... %lu[ms]\n", command.c_str(), millis() - start);

        std::string response;
        while (true)
        {
            if ((response = AtClient<Bg770a<INTERFACE>>::readResponse(timeout)).empty())
            {
                return WioCellularResult::ReadResponseTimeout;
            }

            // Final Result Code
            if (response == "OK")
            {
                printf("FRC> %s\n", response.c_str());
                break;
            }
            if (response == "ERROR" || response.compare(0, 12, "+CME ERROR: ") == 0 || response.compare(0, 12, "+CMS ERROR: ") == 0)
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
     * @param [in] command コマンド
     * @param [in] informationTextHandler information textのハンドラ
     * @param [in] timeout タイムアウト時間[ミリ秒]
     * @return 実行結果
     *
     * 問い合わせコマンドを実行します。
     * informaton textを読み込んだときはinformationTextHandlerを呼び出します。
     */
    WioCellularResult queryCommand(const std::string &command, const std::function<bool(const std::string &response)> &informationTextHandler, int timeout)
    {
        printf("CMD> %s\n", command.c_str());
        const auto start = millis();
        if (!AtClient<Bg770a<INTERFACE>>::writeAndWaitCommand(command, COMMAND_ECHO_TIMEOUT))
        {
            return WioCellularResult::WaitCommandTimeout;
        }
        printf("ECO> %s ... %lu[ms]\n", command.c_str(), millis() - start);

        std::string response;
        while (true)
        {
            if ((response = AtClient<Bg770a<INTERFACE>>::readResponse(timeout)).empty())
            {
                return WioCellularResult::ReadResponseTimeout;
            }

            // Final Result Code
            if (response == "OK")
            {
                printf("FRC> %s\n", response.c_str());
                break;
            }
            if (response == "ERROR" || response.compare(0, 12, "+CME ERROR: ") == 0 || response.compare(0, 12, "+CMS ERROR: ") == 0)
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
     * @param [in] command コマンド
     * @param [in] informationTextHandler information textのハンドラ
     * @param [in] timeout タイムアウト時間[ミリ秒]
     * @return 実行結果
     *
     * 問い合わせコマンドを実行します。
     * informaton textを読み込んだときはinformationTextHandlerを呼び出します。
     */
    WioCellularResult sendCommand(const std::string &command, std::function<bool(const std::string &response)> informationTextHandler, int timeout)
    {
        printf("CMD> %s\n", command.c_str());
        const auto start = millis();
        if (!AtClient<Bg770a<INTERFACE>>::writeAndWaitCommand(command, COMMAND_ECHO_TIMEOUT))
        {
            return WioCellularResult::WaitCommandTimeout;
        }
        printf("ECO> %s ... %lu[ms]\n", command.c_str(), millis() - start);

        std::string response;
        while (true)
        {
            if ((response = AtClient<Bg770a<INTERFACE>>::readResponse(timeout, [](const std::string &response) -> bool
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
     * @param [in] timeout タイムアウト時間[ミリ秒]
     * @return 実行結果
     *
     * 電源をオンします。
     * 処理完了までに10秒程度かかります．
     */
    WioCellularResult powerOn(int timeout)
    {
        WioCellularResult result = WioCellularResult::Ok;

        bool appRdy = false;
        const auto handler = AtClient<Bg770a<INTERFACE>>::registerUrcHandler([&appRdy](const std::string &response) -> bool
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
                AtClient<Bg770a<INTERFACE>>::doWork(timeout - (millis() - start));
                if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                {
                    result = WioCellularResult::RdyTimeout;
                    break;
                }
            }
        }

        AtClient<Bg770a<INTERFACE>>::unregisterUrcHandler(handler);
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
     * @return 実行結果
     *
     * 電源をオフします。
     */
    WioCellularResult powerOff(void)
    {
        getInterface().powerOff();

        return WioCellularResult::Ok;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Undocumented Commands

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
     * > X.X AT+QGMR
     */
    WioCellularResult getModemInfo(std::string *revision)
    {
        if (!revision)
        {
            return WioCellularResult::Ok;
        }
        revision->clear();

        return queryCommand(
            "AT+QGMR", [revision](const std::string &response) -> bool
            {
                *revision = response;
                return true; },
            300);
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
     * > X.X AT+CNUM
     */
    WioCellularResult getPhoneNumber(std::string *phoneNumber)
    {
        if (!phoneNumber)
        {
            return WioCellularResult::Ok;
        }
        phoneNumber->clear();

        return queryCommand(
            "AT+CNUM", [phoneNumber](const std::string &response) -> bool
            {
                if (response.compare(0, 7, "+CNUM: ") == 0)
                {
                    AtParameterParser parser{response.substr(7)};
                    if (parser.size() == 3)
                    {
                        *phoneNumber = parser[1];
                        return true;
                    }
                }
                return false; },
            300);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // General Commands

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
     * > 2.8 AT+GSN - General Commands
     */
    WioCellularResult getIMEI(std::string *imei)
    {
        if (!imei)
        {
            return WioCellularResult::Ok;
        }
        imei->clear();

        return queryCommand(
            "AT+GSN", [imei](const std::string &response) -> bool
            {
                *imei = response;
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
     * > 2.10 AT&F - General Commands
     */
    WioCellularResult factoryDefault(int timeout)
    {
        WioCellularResult result = WioCellularResult::Ok;

        bool appRdy = false;
        const auto handler = AtClient<Bg770a<INTERFACE>>::registerUrcHandler([&appRdy](const std::string &response) -> bool
                                                                             {
            if (response == "APP RDY")
            {
                appRdy = true;
                return true;
            }
            return false; });

        if ((result = executeCommand("AT&F1", 300)) == WioCellularResult::Ok)
        {
            const auto start = millis();
            while (!appRdy)
            {
                AtClient<Bg770a<INTERFACE>>::doWork(timeout - (millis() - start));
                if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                {
                    result = WioCellularResult::RdyTimeout;
                    break;
                }
            }
        }
        AtClient<Bg770a<INTERFACE>>::unregisterUrcHandler(handler);
        if (result != WioCellularResult::Ok)
        {
            return result;
        }

        // Enable Hardware Flow Control
        if ((result = executeCommand("AT+IFC=2,2", 300)) != WioCellularResult::Ok)
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
     * > 2.21 AT+CFUN - General Commands
     */
    WioCellularResult setPhoneFunctionality(int fun)
    {
        return executeCommand(internal::stringFormat("AT+CFUN=%d", fun), 300);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // (U)SIM-Related Commands

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
     * > 5.1 AT+CIMI - (U)SIM-Related Commands
     */
    WioCellularResult getIMSI(std::string *imsi)
    {
        if (!imsi)
        {
            return WioCellularResult::Ok;
        }
        imsi->clear();

        return queryCommand(
            "AT+CIMI", [imsi](const std::string &response) -> bool
            {
                *imsi = response;
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
     * > 5.3 AT+CPIN - (U)SIM-Related Commands
     */
    WioCellularResult getSimState(std::string *state)
    {
        if (!state)
        {
            return WioCellularResult::Ok;
        }
        state->clear();

        return queryCommand(
            "AT+CPIN?", [state](const std::string &response) -> bool
            {
                if (response.compare(0, 7, "+CPIN: ") == 0)
                {
                    *state = response.substr(7);
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
     * > 5.6 AT+QCCID - (U)SIM-Related Commands
     */
    WioCellularResult getSimCCID(std::string *iccid)
    {
        if (!iccid)
        {
            return WioCellularResult::Ok;
        }
        iccid->clear();

        return queryCommand(
            "AT+QCCID", [iccid](const std::string &response) -> bool
            {
                if (response.compare(0, 8, "+QCCID: ") == 0)
                {
                    *iccid = response.substr(8);
                    return true;
                }
                return false; },
            300);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Network Service Commands

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
     * > 6.2 AT+COPS - Network Service Commands
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

        return queryCommand(
            "AT+COPS?", [mode, format, oper, act](const std::string &response) -> bool
            {
                if (response.compare(0, 7, "+COPS: ") == 0)
                {
                    if (mode) *mode = -1;
                    if (format) *format = -1;
                    if (oper) oper->clear();
                    if (act) *act = -1;

                    AtParameterParser parser{response.substr(7)};
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
     * > 6.10 AT+CEDRXS - e-I-DRX Setting
     */
    WioCellularResult setEdrx(int mode, int actType, int edrxCycle)
    {
        if (mode < 0 || 3 < mode)
            return WioCellularResult::ArgumentOutOfRange;
        if (actType != 4 && actType != 5)
            return WioCellularResult::ArgumentOutOfRange;
        if (edrxCycle < 0 || 15 < edrxCycle)
            return WioCellularResult::ArgumentOutOfRange;

        std::string edrxCycleStr;
        for (int i = 0; i < 4; ++i)
            edrxCycleStr += edrxCycle & 0b1000 >> i ? '1' : '0';

        return executeCommand(internal::stringFormat("AT+CEDRXS=%d,%d,\"%s\"", mode, actType, edrxCycleStr.c_str()), 300);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Packet Domain Commands

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
     * > 8.1 AT+CGATT - Packet Domain Commands
     */
    WioCellularResult getPacketDomainState(int *state)
    {
        if (!state)
        {
            return WioCellularResult::Ok;
        }
        *state = -1;

        return queryCommand(
            "AT+CGATT?", [state](const std::string &response) -> bool
            {
                if (response.compare(0, 8, "+CGATT: ") == 0)
                {
                    *state = std::stoi(response.substr(8));
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
     * > 8.2 AT+CGDCONT - Packet Domain Commands
     */
    WioCellularResult setPdpContext(const PdpContext &context)
    {
        return executeCommand(internal::stringFormat("AT+CGDCONT=%d,\"%s\",\"%s\",\"%s\",%d,%d,%d", context.cid, context.pdpType.c_str(), context.apn.c_str(), context.pdpAddr.c_str(), context.dComp, context.hComp, context.ipV4AddrAlloc), 300);
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
     * > 8.2 AT+CGDCONT - Packet Domain Commands
     */
    WioCellularResult getPdpContext(std::vector<PdpContext> *contexts)
    {
        if (!contexts)
        {
            return WioCellularResult::Ok;
        }
        contexts->clear();

        return queryCommand(
            "AT+CGDCONT?", [contexts](const std::string &response) -> bool
            {
                if (response.compare(0, 10, "+CGDCONT: ") == 0)
                {
                    AtParameterParser parser{response.substr(10)};
                    if (parser.size() != 7)
                    {
                        return false;
                    }
                    contexts->push_back({std::stoi(parser[0]), parser[1], parser[2], parser[3], std::stoi(parser[4]), std::stoi(parser[5]), std::stoi(parser[6])});
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
     * > 8.3 AT+CGACT - Packet Domain Commands
     */
    WioCellularResult getPdpContextStatus(std::vector<PdpContextStatus> *statuses)
    {
        if (!statuses)
        {
            return WioCellularResult::Ok;
        }
        statuses->clear();

        return queryCommand(
            "AT+CGACT?", [statuses](const std::string &response) -> bool
            {
                if (response.compare(0, 8, "+CGACT: ") == 0)
                {
                    AtParameterParser parser{response.substr(8)};
                    if (parser.size() != 2)
                    {
                        return false;
                    }
                    statuses->push_back({std::stoi(parser[0]), std::stoi(parser[1])});
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
     * > 8.8 AT+CEREG - Packet Domain Commands
     */
    WioCellularResult setEpsNetworkRegistrationStatusUrc(int n)
    {
        return executeCommand(internal::stringFormat("AT+CEREG=%d", n), 300);
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
     * > 8.8 AT+CEREG - Packet Domain Commands
     */
    WioCellularResult getEpsNetworkRegistrationState(int *state)
    {
        if (!state)
        {
            return WioCellularResult::Ok;
        }
        *state = -1;

        return queryCommand(
            "AT+CEREG?", [state](const std::string &response) -> bool
            {
                if (response.compare(0, 8, "+CEREG: ") == 0)
                {
                    AtParameterParser parser{response.substr(8)};
                    if (parser.size() >= 2)
                    {
                        *state = std::stoi(parser[1]);
                        return true;
                    }
                }
                return false; },
            300);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Socket Commands

    /**
     * @~Japanese
     * @brief ソケットサービスステータス
     */
    struct SocketStatus
    {
        /**
         * @~Japanese
         * @brief 接続ID
         * 0~11
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
         */
        int remotePort;
        /**
         * @~Japanese
         * @brief ローカルポート番号
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
         * 1~5
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
     * @brief ソケットをオープン
     *
     * @param [in] cid PDPコンテキストID
     * @param [in] connectId 接続ID
     * @param [in] serviceType サービスタイプ
     * @param [in] ipAddress IPアドレス
     * @param [in] remotePort リモートポート番号
     * @param [in] localPort ローカルポート番号
     * @return 実行結果
     *
     * ソケットをオープンします。
     *
     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
     * > 2.3.5 AT+QIOPEN - TCP/IP AT Commands
     */
    WioCellularResult openSocket(int cid, int connectId, const std::string &serviceType, const std::string &ipAddress, int remotePort, int localPort)
    {
        WioCellularResult result = WioCellularResult::Ok;

        if (!UrcSocketReceiveAttached_)
        {
            AtClient<Bg770a<INTERFACE>>::registerUrcHandler([this](const std::string &response) -> bool
                                                            {
                if (response.compare(0, 15, "+QIURC: \"recv\",") == 0)
                {
                    const auto connectId = std::stoi(response.substr(15));
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
        const auto handler = AtClient<Bg770a<INTERFACE>>::registerUrcHandler([connectId, &opened, &internalResult](const std::string &response) -> bool
                                                                             {
            const std::string prefix = internal::stringFormat("+QIOPEN: %d,", connectId);
            if (response.compare(0, prefix.size(), prefix) == 0)
            {
                opened = true;
                internalResult = std::stoi(response.substr(prefix.size()));
                return true;
            }
            return false; });

        if ((result = executeCommand(internal::stringFormat("AT+QIOPEN=%d,%d,\"%s\",\"%s\",%d,%d", cid, connectId, serviceType.c_str(), ipAddress.c_str(), remotePort, localPort), 300)) == WioCellularResult::Ok)
        {
            constexpr int timeout = 150000;
            const auto start = millis();
            while (!opened)
            {
                AtClient<Bg770a<INTERFACE>>::doWork(timeout - (millis() - start));
                if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                {
                    result = WioCellularResult::OpenTimeout;
                    break;
                }
            }
        }
        AtClient<Bg770a<INTERFACE>>::unregisterUrcHandler(handler);
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
     * @param [in] connectId 接続ID
     * @return 実行結果
     *
     * ソケットをクローズします。
     *
     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
     * > 2.3.6 AT+QICLOSE - TCP/IP AT Commands
     */
    WioCellularResult closeSocket(int connectId)
    {
        WioCellularResult result = WioCellularResult::Ok;

        if ((result = executeCommand(internal::stringFormat("AT+QICLOSE=%d", connectId), 11000)) != WioCellularResult::Ok)
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
     * @param [in] cid PDPコンテキストID
     * @param [out] statuses ソケットサービスステータス
     * @return 実行結果
     *
     * ソケットサービスステータスを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
     * > 2.3.7 AT+QISTATE - TCP/IP AT Commands
     */
    WioCellularResult getSocketStatus(int cid, std::vector<SocketStatus> *statuses)
    {
        if (!statuses)
        {
            return WioCellularResult::Ok;
        }
        statuses->clear();

        return queryCommand(
            internal::stringFormat("AT+QISTATE=0,%d", cid), [statuses](const std::string &response) -> bool
            {
                if (response.compare(0, 10, "+QISTATE: ") == 0)
                {
                    AtParameterParser parser{response.substr(10)};
                    if (parser.size() != 10)
                    {
                        return false;
                    }
                    statuses->push_back({std::stoi(parser[0]), parser[1], parser[2], std::stoi(parser[3]), std::stoi(parser[4]), std::stoi(parser[5]), std::stoi(parser[6]), std::stoi(parser[7]), std::stoi(parser[8]), parser[9]});
                    return true;
                }
                return false; },
            300);
    }

    /**
     * @~Japanese
     * @brief ソケットへ送信
     *
     * @param [in] connectId 接続ID
     * @param [in] data データ
     * @param [in] dataSize データサイズ
     * @return 実行結果
     *
     * ソケットへ送信します。
     * 値を送る必要が無いときはdata=nullptrもしくはdataSize<=0を指定できます。
     *
     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
     * > 2.3.8 AT+QISEND - TCP/IP AT Commands
     */
    WioCellularResult sendSocket(int connectId, const void *data, size_t dataSize)
    {
        if (!data || dataSize <= 0)
        {
            return WioCellularResult::Ok;
        }

        return sendCommand(
            internal::stringFormat("AT+QISEND=%d,%d", connectId, dataSize), [this, data, dataSize](const std::string &response) -> bool
            {
                if (response == "> ")
                {
                    AtClient<Bg770a<INTERFACE>>::writeBinary(data, dataSize);
                    AtClient<Bg770a<INTERFACE>>::readBinaryDiscard(dataSize, COMMAND_ECHO_TIMEOUT);
                    return true;
                }
                return false; },
            120000);
    }

    /**
     * @~Japanese
     * @brief ソケットへ送信
     *
     * @param [in] connectId 接続ID
     * @param [in] data データ
     * @return 実行結果
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
     * @param [in] connectId 接続ID
     * @param [out] availableSize 未読のデータサイズ
     * @return 実行結果
     *
     * ソケットから未読のデータサイズを取得します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
     * > 2.3.9 AT+QIRD - TCP/IP AT Commands
     */
    WioCellularResult getSocketReceiveAvailable(int connectId, size_t *availableSize)
    {
        if (!availableSize)
        {
            return WioCellularResult::Ok;
        }
        *availableSize = -1;

        return queryCommand(
            internal::stringFormat("AT+QIRD=%d,0", connectId), [availableSize](const std::string &response) -> bool
            {
                if (response.compare(0, 7, "+QIRD: ") == 0)
                {
                    AtParameterParser parser{response.substr(7)};
                    if (parser.size() < 3)
                    {
                        return false;
                    }
                    *availableSize = std::stoi(parser[2]);
                    return true;
                }
                return false; },
            120000);
    }

    /**
     * @~Japanese
     * @brief ソケットから受信
     *
     * @param [in] connectId 接続ID
     * @param [in,out] data データ
     * @param [in] dataSize データサイズ
     * @param [out] readDataSize 受信したデータサイズ
     * @return 実行結果
     *
     * ソケットから受信します。
     * 受信したデータが無いときは*readDataSize=0を返します。
     * 値を得る必要が無いときはnullptrを指定できます。
     *
     * > BG770A-GL&BG95xA-GL TCP/IP Application Note @n
     * > 2.3.9 AT+QIRD - TCP/IP AT Commands
     */
    WioCellularResult receiveSocket(int connectId, void *data, size_t dataSize, size_t *readDataSize)
    {
        if (!data || dataSize <= 0)
        {
            return WioCellularResult::Ok;
        }
        if (readDataSize)
        {
            *readDataSize = 0;
        }

        UrcSocketReceiveNofity_[connectId] = false;

        return queryCommand(
            internal::stringFormat("AT+QIRD=%d,%d", connectId, dataSize), [this, data, dataSize, readDataSize](const std::string &response) -> bool
            {
                if (response.compare(0, 7, "+QIRD: ") == 0)
                {
                    AtParameterParser parser{response.substr(7)};
                    if (parser.size() < 1)
                    {
                        return false;
                    }
                    const size_t actualDataSize = std::stoi(parser[0]);
                    assert(actualDataSize <= dataSize);
                    if (actualDataSize >= 1)
                    {
                        if (!AtClient<Bg770a<INTERFACE>>::readBinary(data, actualDataSize, 120000))
                        {
                            return false;
                        }
                    }
                    if (readDataSize)
                    {
                        *readDataSize = actualDataSize;
                    }
                    return true;
                }
                return false; },
            120000);
    }

    /**
     * @~Japanese
     * @brief ソケットから受信
     *
     * @param [in] connectId 接続ID
     * @param [in,out] data データ
     * @param [in] dataSize データサイズ
     * @param [out] readDataSize 受信したデータサイズ
     * @param [in] timeout タイムアウト時間[ミリ秒]
     * @return 実行結果
     *
     * ソケットから受信します。
     * 値を得る必要が無いときはnullptrを指定できます。
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
                AtClient<Bg770a<INTERFACE>>::doWork(timeout - (millis() - start));
                if (timeout >= 0 && millis() - start >= static_cast<uint32_t>(timeout))
                {
                    return WioCellularResult::ReceiveTimeout;
                }
            } while (!UrcSocketReceiveNofity_[connectId]);
        }
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
     * > 2.1.1.8 AT+QCFG="psm/urc" - Enable/Disable PSM Entering Indication
     */
    WioCellularResult setPsmEnteringIndicationUrc(bool enable)
    {
        return executeCommand(internal::stringFormat("AT+QCFG=\"psm/urc\",%d", enable ? 1 : 0), 300);
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
     * > 2.1.1.8 AT+QCFG="psm/urc" - Enable/Disable PSM Entering Indication
     */
    WioCellularResult setPsm(int mode, int periodicTau, int activeTau)
    {
        if (mode < 0 || 1 < mode)
            return WioCellularResult::ArgumentOutOfRange;
        if (periodicTau < 0 || 32 <= periodicTau / 36000)
            return WioCellularResult::ArgumentOutOfRange;
        if (activeTau < 0 || 32 <= activeTau / 360)
            return WioCellularResult::ArgumentOutOfRange;

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

        return executeCommand(internal::stringFormat("AT+CPSMS=%d,,,\"%s\",\"%s\"", mode, periodicStr.c_str(), activeStr.c_str()), 4000);
    }
};

#endif // BG770A_HPP
