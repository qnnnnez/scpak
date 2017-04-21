#pragma once
#include "pakfile.h"
#include <string>

namespace scpak
{
    PakFile pack(const std::string &dirPath,
        bool packText = true, bool packTexture = true, bool packFont = true,
        bool packSound = true);

    void pack_string(const std::string &value, PakItem &item);
    void pack_bitmapFont(const std::string &inputDir, PakItem &item);
    void pack_soundBuffer(const std::string &inputDir, PakItem &item);

    int calcMipmapSize(int width, int height, int level = 0);
    int generateMipmap(int width, int height, int level, unsigned char *image);
}

