#pragma once
#include "pakfile.h"
#include <string>
#include <functional>
#include <map>

namespace scpak
{
    typedef std::function<void(const std::string &inputDir, PakItem &output, const std::string &meta)> packer_type;
    PakFile pack(const std::string &dirPath,
        const std::map<std::string, packer_type> &packers,
        const packer_type &default_packer);
    PakFile pack(const std::string &dirPath,
        bool packText = false,
        bool packTexture = false,
        bool packFont = false,
        bool packSound = false);
    PakFile packAll(const std::string &dirPath);

    void pack_raw(const std::string &inputDir, PakItem &item);
    void pack_string(const std::string &inputDir, PakItem &item);
    void pack_bitmapFont(const std::string &inputDir, PakItem &item);
    void pack_texture(const std::string &inputDir, PakItem &item, const std::string &meta);
    void pack_soundBuffer(const std::string &inputDir, PakItem &item);
}

