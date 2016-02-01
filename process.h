#pragma once
#include "pakfile.h"
#include <string>

namespace scpak
{
    PakFile pack(const std::string &dirPath,
                 bool packText=true, bool packTexture=true, bool packFont=true);
    void unpack(const PakFile &pak, const std::string &dirPath,
                bool unpackText=true, bool unpackTexture=true, bool unpackFont=true);

    int calcMipmapSize(int width, int height, int level=0);
    int generateMipmap(int width, int height, int level, unsigned char *image);

    void unpack_string(const std::string &fileName, const PakItem &item);
    void pack_string(const std::string &value, PakItem &item);

    void unpack_bitmapFont(const std::string &outputDir, const PakItem &item);
    void pack_bitmapFont(const std::string &inputDir, PakItem &item);

    bool isPowerOfTwo(int number);

    struct Vector2f
    {
        float x;
        float y;
    };

    struct GlyphInfo
    {
        int unicode;
        Vector2f texCoord1;
        Vector2f texCoord2;
        Vector2f offset;
        float width;
    };
}

