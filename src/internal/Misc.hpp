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

} // namespace internal

#endif // MISC_HPP
