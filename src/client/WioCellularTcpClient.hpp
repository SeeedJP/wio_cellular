/*
 * WioCellularTcpClient.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef WIOCELLULARTCPCLIENT_HPP
#define WIOCELLULARTCPCLIENT_HPP

#include "../WioCellular.hpp"
#include <Client.h>
#include <array>
#include <queue>

/**
 * @~Japanese
 * @brief TCPクライアント
 *
 * @tparam MODULE モジュールのクラス
 *
 * TCPクライアントのクラスです。
 */
template <typename MODULE>
class WioCellularTcpClient : public Client
{
protected:
    static constexpr size_t RECEIVE_MAX_LENGTH = 1500;

    MODULE &Module_;
    int PdpContextId_;
    int ConnectId_;
    bool Connected_;
    std::queue<uint8_t> ReceiveQueue_;
    std::array<uint8_t, RECEIVE_MAX_LENGTH> ReceiveBuffer_;

public:
    /**
     * @~Japanese
     * @brief コンストラクタ
     *
     * @param [in] module モジュールのインスタンス。
     * @param [in] pdpContextId PDPコンテキスト。
     * @param [in] connectId 接続ID。
     *
     * コンストラクタ。
     */
    WioCellularTcpClient(MODULE &module, int pdpContextId, int connectId) : Module_{module},
                                                                            PdpContextId_{pdpContextId},
                                                                            ConnectId_{connectId},
                                                                            Connected_{false}
    {
    }

    /**
     * @~Japanese
     * @brief デストラクタ
     *
     * デストラクタ。
     */
    virtual ~WioCellularTcpClient(void)
    {
        if (Connected_)
            stop();
    }

    /**
     * @~Japanese
     * @brief TCPサーバーに接続
     *
     * @param [in] ip IPアドレス。
     * @param [in] port ポート番号。
     * @retval 1 成功
     * @retval 0 エラー
     *
     * TCPサーバーに接続します。
     */
    virtual int connect(IPAddress ip, uint16_t port)
    {
        if (Connected_)
            return 0;

        String ipStr = String(ip[0]);
        ipStr += ".";
        ipStr += String(ip[1]);
        ipStr += ".";
        ipStr += String(ip[2]);
        ipStr += ".";
        ipStr += String(ip[3]);

        return connect(ipStr.c_str(), port);
    }

    /**
     * @~Japanese
     * @brief TCPサーバーに接続
     *
     * @param [in] host ホスト名。
     * @param [in] port ポート番号。
     * @retval 1 成功
     * @retval 0 エラー
     *
     * TCPサーバーに接続します。
     */
    virtual int connect(const char *host, uint16_t port)
    {
        if (Connected_)
            return 0;

        if (Module_.openSocket(PdpContextId_, ConnectId_, "TCP", host, port, 0) != WioCellularResult::Ok)
            return 0;

        Connected_ = true;

        return 1;
    }

    /**
     * @~Japanese
     * @brief TCPサーバーへ送信
     *
     * @param [in] data データ。
     * @return 送信したデータサイズ。
     *
     * TCPサーバーへ送信します。
     */
    virtual size_t write(uint8_t data)
    {
        return write(&data, 1);
    }

    /**
     * @~Japanese
     * @brief TCPサーバーへ送信
     *
     * @param [in] buf データ。
     * @param [in] size データサイズ。
     * @return 送信したデータサイズ。
     *
     * TCPサーバーへ送信します。
     */
    virtual size_t write(const uint8_t *buf, size_t size)
    {
        if (!Connected_)
            return 0;

        if (Module_.sendSocket(ConnectId_, buf, size) != WioCellularResult::Ok)
            return 0;

        return size;
    }

    /**
     * @~Japanese
     * @brief 未読のデータサイズを取得
     *
     * @retval >=0 未読のデータサイズ
     * @retval <0 エラー
     *
     * TCPサーバーから受信した、未読のデータサイズを取得します。
     * エラーのときは負の値を返します。
     */
    virtual int available(void)
    {
        if (!Connected_)
            return -1;

        size_t size;
        if (Module_.receiveSocket(ConnectId_, ReceiveBuffer_.data(), ReceiveBuffer_.size(), &size) != WioCellularResult::Ok)
            return -1;

        for (size_t i = 0; i < size; ++i)
            ReceiveQueue_.push(ReceiveBuffer_[i]);

        return ReceiveQueue_.size();
    }

    /**
     * @~Japanese
     * @brief TCPサーバーから受信
     *
     * @retval >=0 受信データ
     * @retval <0 受信データ無し
     *
     * TCPサーバーから受信します。
     * 受信データが無いときは負の値を返します。
     */
    virtual int read(void)
    {
        if (!Connected_)
            return -1;

        const int actualSize = available();
        if (actualSize <= 0)
            return -1;

        const uint8_t data = ReceiveQueue_.front();
        ReceiveQueue_.pop();

        return data;
    }

    /**
     * @~Japanese
     * @brief TCPサーバーから受信
     *
     * @param [in,out] buf データ。
     * @param [in] size データサイズ。
     * @retval >=0 受信したデータサイズ
     * @retval <0 エラー
     *
     * TCPサーバーから受信します。
     * エラーのときは負の値を返します。
     */
    virtual int read(uint8_t *buf, size_t size)
    {
        if (!Connected_)
            return -1;

        const int actualSize = available();
        if (actualSize < 0)
            return -1;

        const int popSize = static_cast<size_t>(actualSize) <= size ? actualSize : size;
        for (int i = 0; i < popSize; ++i)
        {
            buf[i] = ReceiveQueue_.front();
            ReceiveQueue_.pop();
        }

        return popSize;
    }

    /**
     * @~Japanese
     * @brief TCPサーバーから先読み受信
     *
     * @retval >=0 受信データ
     * @retval <0 受信データ無し
     *
     * TCPサーバーから受信したデータを先読みします。
     * 受信データが無いときは負の値を返します。
     */
    virtual int peek(void)
    {
        if (!Connected_)
            return -1;

        const int actualSize = available();
        if (actualSize <= 0)
            return -1;

        return ReceiveQueue_.front();
    }

    /**
     * @~Japanese
     * @brief 受信データを破棄
     *
     * TCPサーバーから受信したデータを破棄します。
     */
    virtual void flush(void)
    {
        if (!Connected_)
            return;

        available();

        while (!ReceiveQueue_.empty())
            ReceiveQueue_.pop();
    }

    /**
     * @~Japanese
     * @brief TCPサーバーを切断
     *
     * TCPサーバーを切断します。
     */
    virtual void stop(void)
    {
        if (!Connected_)
            return;

        Module_.closeSocket(ConnectId_);

        while (!ReceiveQueue_.empty())
            ReceiveQueue_.pop();

        Connected_ = false;
    }

    /**
     * @~Japanese
     * @brief TCPサーバーの接続状態を取得
     *
     * @retval 1 接続
     * @retval 0 切断
     *
     * TCPサーバーの接続状態を取得します。
     */
    virtual uint8_t connected(void)
    {
        return Connected_ ? 1 : 0;
    }

    /**
     * @~Japanese
     * @brief TCPサーバーの接続状態を取得
     *
     * @retval 1 接続
     * @retval 0 切断
     *
     * TCPサーバーの接続状態を取得します。
     */
    virtual operator bool(void)
    {
        return connected();
    }
};

#endif // WIOCELLULARTCPCLIENT_HPP
