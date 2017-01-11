#include "wav.h"

namespace scpak
{
    const std::string WavHeader::chunkIdMagic = "RIFF";
    const std::string WavHeader::formatMagic = "WAVE";
    const std::string WavHeader::subchunk1LabelMagic = "fmt ";
    const std::uint32_t WavHeader::subchunk1SizeMagic = 16;
    const std::uint16_t WavHeader::audioFormatMagic = 1;
    const std::uint16_t WavHeader::blockAlighMagic = 4;
    const std::string WavHeader::subchunk2LabelMagic = "data";
}