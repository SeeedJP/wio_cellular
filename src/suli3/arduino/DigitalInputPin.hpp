/*
 * DigitalInputPin.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef DIGITALINPUTPIN_HPP
#define DIGITALINPUTPIN_HPP

#include <Arduino.h>

namespace suli3
{
    namespace arduino
    {
        /**
         * @~Japanese
         * @brief 1ピン デジタル入力
         *
         * 1ピンのデジタル入力を扱うクラスです。
         */
        template <int PIN>
        class DigitalInputPin
        {
        public:
            /**
             * @~Japanese
             * @brief デジタル入力を開始
             *
             * @param [in] mode 入力モード
             *
             * ピンをデジタル入力に初期化します。
             * modeには[pinMode関数](https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/)のmode値を指定します。例えばINPUTです。
             */
            void begin(int mode)
            {
                pinMode(PIN, mode);
            }

            /**
             * @~Japanese
             * @brief デジタル入力を終了
             *
             * ピンをデジタル入力から解放します。
             */
            void end(void)
            {
                pinMode(PIN, NO_CONNECT);
            }

            /**
             * @~Japanese
             * @brief ピンの状態を読み込む
             *
             * @retval 0 ピンがLOW
             * @retval 1 ピンがHIGH
             *
             * ピンの状態を読み込みます。
             */
            int read(void) const
            {
                static_assert(LOW == 0 && HIGH == 1, "Invalid LOW/HIGH");

                return digitalRead(PIN);
            }
        };

    }
}

#endif // DIGITALINPUTPIN_HPP
