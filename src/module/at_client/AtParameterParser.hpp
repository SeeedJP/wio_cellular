/*
 * AtParameterParser.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef ATPARAMETERPARSER_HPP
#define ATPARAMETERPARSER_HPP

#include <string>
#include <vector>

namespace wiocellular
{
    namespace module
    {
        namespace at_client
        {

            /**
             * @~Japanese
             * @brief ATパラメータのパーサー
             *
             * ATレスポンスのパラメータを解析するクラスです。
             */
            class AtParameterParser
            {
            private:
                std::vector<std::string> Parameters_;

            public:
                /**
                 * @~Japanese
                 * @brief コンストラクタ
                 *
                 * @param [in] parameters パラメータ。
                 *
                 * コンストラクタ。
                 * パラメータを解析します。
                 */
                explicit AtParameterParser(const std::string &parameters)
                {
                    std::string parameter;
                    bool inString = false;
                    for (const auto c : parameters)
                    {
                        if (!inString)
                        {
                            switch (c)
                            {
                            case ',':
                                Parameters_.push_back(parameter);
                                parameter.clear();
                                break;
                            case '"':
                                inString = true;
                                break;
                            default:
                                parameter.push_back(c);
                                break;
                            }
                        }
                        else
                        {
                            switch (c)
                            {
                            case '"':
                                inString = false;
                                break;
                            default:
                                parameter.push_back(c);
                                break;
                            }
                        }
                    }
                    if (!parameter.empty() || (parameters.size() >= 1 && parameters.rbegin()[0] == ','))
                    {
                        Parameters_.push_back(parameter);
                    }
                }

                /**
                 * @~Japanese
                 * @brief パラメータ数を取得
                 *
                 * @return パラメータ数。
                 *
                 * パラメータ数を取得します。
                 */
                size_t size(void) const
                {
                    return Parameters_.size();
                }

                /**
                 * @~Japanese
                 * @brief パラメータ要素を取得
                 *
                 * @param [in] index インデックス。
                 * @return パラメータ要素。
                 *
                 * 指定したインデックスのパラメータ要素を取得します。
                 */
                std::string operator[](size_t index) const
                {
                    assert(index < Parameters_.size());

                    return Parameters_[index];
                }
            };

        }
    }
}

#endif // ATPARAMETERPARSER_HPP
