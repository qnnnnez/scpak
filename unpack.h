#pragma once
#include "pakfile.h"
#include <string>
#include <map>
#include <functional>
#include <iostream>

namespace scpak
{
    void unpack_raw(const std::string &outputDir, const PakItem &item);
    void unpack_string(const std::string &outputDir, const PakItem &item);
    void unpack_bitmapFont(const std::string &outputDir, const PakItem &item);
    void unpack_texture(const std::string &outputDir, const PakItem &item, std::iostream &meta);
    void unpack_soundBuffer(const std::string &outputDir, const PakItem &item);

    typedef std::function<void(const std::string &outputDir, const PakItem &item, std::iostream &meta)> unpacker_type;
    void unpack(
        const PakFile &pak, const std::string &dirPath,
        const std::map<std::string, unpacker_type> &unpackers,
        const unpacker_type &default_unpacker);
    void unpack(const PakFile &pak, const std::string &dirPath,
        bool unpack_text = false,
        bool unpack_bitmapFont = false,
        bool unpack_texture = false,
        bool unpack_soundBuffer = false);
}

