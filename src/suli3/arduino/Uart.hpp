/*
 * Uart.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef UART_HPP
#define UART_HPP

#include <Arduino.h>

namespace suli3
{
    namespace arduino
    {

        /**
         * @~Japanese
         * @brief UART通信
         *
         * @tparam T UART通信の実クラス
         *
         * UART通信のクラスです。
         */
        template <typename T>
        class Uart
        {
        private:
            T &RealUart_;

        public:
            /**
             * @~Japanese
             * @brief コンストラクタ
             *
             * @param [in] uart UARTのインスタンス
             *
             * コンストラクタ。
             * uartにUARTのインスタンスを指定します。
             */
            explicit Uart(T &uart) : RealUart_{uart}
            {
            }

            /**
             * @~Japanese
             * @brief UARTを開始
             *
             * @param [in] baudrate ボーレート
             *
             * UARTを初期化します。
             * baudrateにボーレートを指定します。例えば9600。
             */
            void begin(int baudrate)
            {
                RealUart_.begin(baudrate);
            }

            /**
             * @~Japanese
             * @brief UARTを終了
             *
             * UARTを解放します。
             */
            void end(void)
            {
                RealUart_.end();
            }

            /**
             * @~Japanese
             * @brief 1バイト読み込み
             *
             * @retval <0 受信データ無し
             * @retval >=0 受信データ
             *
             * UARTに受信したデータを読み込みます。
             * 受信データが無いときは負の値を返します。
             */
            int read(void)
            {
                return RealUart_.read();
            }

            /**
             * @~Japanese
             * @brief 1バイト書き込み
             *
             * @param [in] data 送信データ
             *
             * UARTへ送信するデータを書き込みます。
             */
            void write(int data)
            {
                RealUart_.write(data);
            }
        };

    }
}

#endif // UART_HPP
