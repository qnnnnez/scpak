#pragma once
#include "pakfile.h"
#include <string>

namespace scpak
{
    void unpack(const PakFile &pak, const std::string &dirPath,
        bool unpackText = true, bool unpackTexture = true, bool unpackFont = true,
        bool unpackSound = true);
    void unpack_string(const std::string &fileName, const PakItem &item);
    void unpack_bitmapFont(const std::string &outputDir, const PakItem &item);
    void unpack_soundBuffer(const std::string &outputDir, const PakItem &item);
}

