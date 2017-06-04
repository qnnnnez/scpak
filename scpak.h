#pragma once
#include <cstdint>
#include <cmath>


namespace scpak
{
    extern const char *Version;
    extern const char *PakInfoFileName;

    typedef std::uint8_t byte;

    inline bool isPowerOfTwo(int n)
    {
        return (n & (n - 1)) == 0;
    }

    struct Vector2f
    {
        std::float_t x;
        std::float_t y;
    };

    struct GlyphInfo
    {
        std::int32_t unicode;
        Vector2f texCoord1;
        Vector2f texCoord2;
        Vector2f offset;
        std::float_t width;
    };

    class BaseException
    {
    public:
        virtual const char * what() const = 0;
        virtual ~BaseException() { };
    };
}
