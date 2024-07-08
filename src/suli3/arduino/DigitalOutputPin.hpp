/*
 * DigitalOutputPin.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef DIGITALOUTPUTPIN_HPP
#define DIGITALOUTPUTPIN_HPP

#include <Arduino.h>
#include <cassert>

namespace suli3
{
    namespace arduino
    {
        /**
         * @~Japanese
         * @brief 1ピン デジタル出力
         *
         * 1ピンのデジタル出力を扱うクラスです。
         */
        template <int PIN>
        class DigitalOutputPin
        {
        public:
            /**
             * @~Japanese
             * @brief デジタル出力を開始
             *
             * @param [in] mode 出力モード
             * @param [in] initialValue ピンの状態
             *
             * ピンをデジタル出力に初期化します。
             * modeには[pinMode関数](https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/)のmode値を指定します。例えばOUTPUTです。
             * initialValueにはピンの状態をLOWにするときは0、HIGHにするときは1を指定します。
             */
            void begin(int mode, int initialValue = -1)
            {
                assert(initialValue == -1 || initialValue == 0 || initialValue == 1);
                static_assert(LOW == 0 && HIGH == 1, "Invalid LOW/HIGH");

                if (initialValue >= 0)
                {
                    digitalWrite(PIN, initialValue);
                }
                pinMode(PIN, mode);
            }

            /**
             * @~Japanese
             * @brief デジタル出力を終了
             *
             * ピンをデジタル出力から解放します。
             */
            void end(void)
            {
                pinMode(PIN, NO_CONNECT);
            }

            /**
             * @~Japanese
             * @brief ピンの状態を書き込む
             *
             * @param [in] value ピンの状態
             *
             * ピンの状態を書き込みます。
             * valueにはピンの状態をLOWにするときは0、HIGHにするときは1を指定します。
             */
            void write(int value)
            {
                assert(value == 0 || value == 1);
                static_assert(LOW == 0 && HIGH == 1, "Invalid LOW/HIGH");

                digitalWrite(PIN, value);
            }
        };

    }
}

#endif // DIGITALOUTPUTPIN_HPP
