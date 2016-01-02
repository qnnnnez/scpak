#pragma once
#include "pakfile.h"
#include <string>

namespace scpak
{
    PakFile pack(const std::string &dirPath);
    void unpack(const PakFile &pak, const std::string &dirPath);
}

