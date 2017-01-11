#pragma once
#include <cstdint>


namespace scpak
{
    typedef std::uint8_t byte;

    inline bool isPowerOfTwo(int n)
    {
        return (n & (n - 1)) == 0;
    }
}