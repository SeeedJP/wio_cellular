/*
 * Misc.hpp
 * Copyright (C) Seeed K.K.
 * MIT License
 */

#ifndef MISC_HPP
#define MISC_HPP

#include <cstdio>
#include <string>

namespace internal
{

    template <class... Args>
    static std::string stringFormat(const std::string &format, Args... args)
    {
        const size_t len = snprintf(nullptr, 0, format.c_str(), args...);

        char buf[len + 1];
        snprintf(buf, len + 1, format.c_str(), args...);
        return {buf, len};
    }

    template <typename T>
    static bool stringStartsWith(const std::string &str, const T &prefix, std::string *rest = nullptr)
    {
        const auto prefixLen = sizeof(prefix) - 1;

        if (str.compare(0, prefixLen, prefix) == 0)
        {
            if (rest)
            {
                *rest = str.substr(prefixLen);
            }
            return true;
        }
        else
        {
            return false;
        }
    }

} // namespace internal

#endif // MISC_HPP
