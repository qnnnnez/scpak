#pragma once
#include "pakfile.h"
#include <string>
#include <map>
#include <functional>
#include <iostream>

namespace scpak
{
    typedef std::function<std::string(const std::string &outputDir, const PakItem &item)> unpacker_type;
    void unpack(
        const PakFile &pak, const std::string &dirPath,
        const std::map<std::string, unpacker_type> &unpackers,
        const unpacker_type &default_unpacker);
    void unpack(const PakFile &pak, const std::string &dirPath,
        bool unpack_text = false,
        bool unpack_bitmapFont = false,
        bool unpack_texture = false,
        bool unpack_sound = false);
    void unpackAll(const PakFile &pak, const std::string &dirPath);

    void unpack_raw(const std::string &outputDir, const PakItem &item);
    void unpack_string(const std::string &outputDir, const PakItem &item);
    void unpack_bitmapFont(const std::string &outputDir, const PakItem &item);
    std::string unpack_texture(const std::string &outputDir, const PakItem &item);
    void unpack_soundBuffer(const std::string &outputDir, const PakItem &item);
}

