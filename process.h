#pragma once
#include "pakfile.h"
#include <string>

namespace scpak
{
    PakFile pack(const std::string &dirPath);
    void unpack(const PakFile &pak, const std::string &dirPath);

    int calcMipmapSize(int width, int height, int level=0);
    int generateMipmap(int width, int height, int level, unsigned char *image);
}

