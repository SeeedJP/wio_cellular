/*
 * WioCellularResult.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef WIOCELLULARRESULT_HPP
#define WIOCELLULARRESULT_HPP

/**
 * @~Japanese
 * 実行結果
 */
enum class WioCellularResult
{
    /**
     * @~Japanese
     * 成功
     */
    Ok = 0,
    /**
     * @~Japanese
     * コマンドエコーの待機でタイムアウト
     */
    WaitCommandTimeout = 1,
    /**
     * @~Japanese
     * レスポンス読み込みでタイムアウト
     */
    ReadResponseTimeout = 2,
    /**
     * @~Japanese
     * モジュールがコマンドを拒否
     */
    CommandRejected = 3,
    /**
     * @~Japanese
     * RDYの待機でタイムアウト
     */
    RdyTimeout = 4,
    /**
     * @~Japanese
     * オープン完了待ちでタイムアウト
     */
    OpenTimeout = 5,
    /**
     * @~Japanese
     * オープンでエラー
     */
    OpenError = 6,
    /**
     * @~Japanese
     * 受信でタイムアウト
     */
    ReceiveTimeout = 7,
    /**
     * @~Japanese
     * 起動しない
     */
    NotActivate = 8,
    /**
     * @~Japanese
     * 引数が範囲外
     */
    ArgumentOutOfRange = 9,
};

/**
 * @~Japanese
 * @brief 実行結果を文字列に変換
 *
 * @param [in] result 実行結果。
 * @return 実行結果の文字列。
 *
 * 実行結果を文字列に変換します。
 */
static constexpr const char *WioCellularResultToString(WioCellularResult result)
{
    return result == WioCellularResult::Ok ? "Ok" : result == WioCellularResult::WaitCommandTimeout ? "WaitCommandTimeout"
                                                : result == WioCellularResult::ReadResponseTimeout  ? "ReadResponseTimeout"
                                                : result == WioCellularResult::CommandRejected      ? "CommandRejected"
                                                : result == WioCellularResult::RdyTimeout           ? "RdyTimeout"
                                                : result == WioCellularResult::OpenTimeout          ? "OpenTimeout"
                                                : result == WioCellularResult::OpenError            ? "OpenError"
                                                : result == WioCellularResult::ReceiveTimeout       ? "ReceiveTimeout"
                                                : result == WioCellularResult::NotActivate          ? "NotActivate"
                                                : result == WioCellularResult::ArgumentOutOfRange   ? "ArgumentOutOfRange"
                                                                                                    : "Unknown";
}

#endif // WIOCELLULARRESULT_HPP
