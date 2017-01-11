#pragma once
#include <vector>
#include <string>
#include <algorithm>

#include "scpak.h"
#include "binaryio.h"


namespace scpak
{
    typedef struct WavHeaderStruct
    {
        char chunkId[4]; // "RIFF"
        std::uint32_t chunkSize;
        char format[4]; // "WAVE"

        char subchunk1Label[4]; // "fmt "
        std::uint32_t subchunk1Size; // 16
        std::uint16_t audioFormat; // 1 - PCM
        std::uint16_t channelCount;
        std::uint32_t sampleRate;
        std::uint32_t byteRate; // per channel
        std::uint16_t blockAlign; // 4
        std::uint16_t bitsPerSample;
        
        char subchunk2Label[4]; // "data"
        std::uint32_t subchunk2Size; // second * byte_rate * channel_count


        static const std::string chunkIdMagic;
        static const std::string formatMagic;
        static const std::string subchunk1LabelMagic;
        static const std::uint32_t subchunk1SizeMagic;
        static const std::uint16_t audioFormatMagic;
        static const std::uint16_t blockAlighMagic;
        static const std::string subchunk2LabelMagic;

        bool checkMagicValues() const
        {
            return
                chunkSize == subchunk2Size + 36 &&
                chunkId == chunkIdMagic &&
                format == formatMagic &&
                subchunk1Label == subchunk1LabelMagic &&
                subchunk1Size == subchunk1SizeMagic &&
                audioFormat == audioFormatMagic &&
                blockAlign == blockAlighMagic &&
                subchunk2Label == subchunk2LabelMagic;
        }

        static void SetMagicValues(struct WavHeaderStruct &header)
        {
            auto copyString = [](std::string src, char *dst)
            {
                std::copy(src.begin(), src.end(), dst);
            };

            copyString(chunkIdMagic, header.chunkId);
            copyString(formatMagic, header.format);
            copyString(subchunk1LabelMagic, header.subchunk1Label);
            header.subchunk1Size = subchunk1SizeMagic;
            header.audioFormat = audioFormatMagic;
            header.blockAlign = blockAlighMagic;
            copyString(subchunk2LabelMagic, header.subchunk2Label);
        }
    } WavHeader;
}