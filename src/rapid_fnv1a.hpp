// Copyright © 2020 igor . ikonopistsev at gmail
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace rapid {

// x86
struct fnv1a
{
    std::uint32_t salt{ 0x811c9dc5 };

    constexpr operator std::uint32_t() const noexcept
    {
        return salt;
    }

    constexpr auto operator()(const char *ptr) const noexcept
    {
        auto hval = salt;
        while (*ptr != '\0')
        {
            hval ^= static_cast<std::uint32_t>(*ptr++);
            hval += (hval << 1) + (hval << 4) +
                (hval << 7) + (hval << 8) + (hval << 24);
        }
        return hval;
    }

    constexpr auto operator()(const char *p, const char *e) const noexcept
    {
        auto hval = salt;
        while (p < e)
        {
            hval ^= static_cast<std::uint32_t>(*p++);
            hval += (hval << 1) + (hval << 4) +
                (hval << 7) + (hval << 8) + (hval << 24);
        }
        return hval;
    }

    constexpr auto operator()(const char *ptr, std::size_t len) const noexcept
    {
        auto p = static_cast<const char*>(ptr);
        return this->operator()(p, p + len);
    }
};

} // namespace rapid
