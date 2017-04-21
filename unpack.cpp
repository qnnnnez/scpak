#include "unpack.h"
#include "native.h"
#include "wav.h"
#include <stdexcept>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"


namespace scpak
{
    void unpack(const PakFile &pak, const std::string &dirPath, bool unpackText, bool unpackTexture, bool unpackFont, bool unpackSound)
    {
        std::string dirPathSafe = dirPath;
        if (*dirPathSafe.rbegin() != pathsep)
            dirPathSafe += pathsep;
        // find all the directories we possibly need to create
        std::set<std::string> directoriesToCreate;
        directoriesToCreate.insert(dirPath);
        for (const PakItem &item : pak.contents())
        {
            std::string filePath = item.name;
            std::size_t pend = filePath.rfind('/');
            if (pend == std::string::npos)
                continue;
            std::size_t p = 0;
            while (p != pend)
            {
                p = filePath.find('/', p + 1);
                directoriesToCreate.insert(dirPathSafe + filePath.substr(0, p));
            }
        }
        // create directories if necessary
        for (const std::string &dir : directoriesToCreate)
            if (!pathExists(dir.c_str()))
                createDirectory(dir.c_str());
        // unpack contents
        std::vector<std::string> infoLines;
        for (const PakItem &item : pak.contents())
        {
            std::stringstream lineBuffer;
            lineBuffer << item.name << ':' << item.type;
            std::string itemType = item.type;
            if (unpackText && itemType == "System.String")
            {
                unpack_string(dirPathSafe, item);
            }
            else if (unpackText && itemType == "System.Xml.Linq.XElement")
            {
                unpack_string(dirPathSafe, item);
            }
            else if (unpackTexture && itemType == "Engine.Graphics.Texture2D")
            {
                std::string fileName = dirPathSafe + item.name + ".tga";
                const byte *data = item.data.data();
                const int &width = *reinterpret_cast<const int*>(data);
                const int &height = *(reinterpret_cast<const int*>(data) + 1);
                const int &mipmapLevel = *(reinterpret_cast<const int*>(data) + 2);
                const void *imageData = reinterpret_cast<const void*>(data + sizeof(int) * 3);
                stbi_write_tga(fileName.c_str(), width, height, 4, imageData);

                lineBuffer << ':' << mipmapLevel;
            }
            else if (unpackFont && itemType == "Engine.Media.BitmapFont")
            {
                unpack_bitmapFont(dirPathSafe, item);
            }
            else if (unpackSound && itemType == "Engine.Audio.SoundBuffer")
            {
                unpack_soundBuffer(dirPathSafe, item);
            }
            else
            {
            unpack_raw:
                // just write out raw data
                std::ofstream fout(dirPathSafe + item.name, std::ios::binary);
                fout.write(reinterpret_cast<const char*>(item.data.data()), item.length);
                fout.close();
            }
            infoLines.push_back(lineBuffer.str());
        }
        // write info file - we need to repack it later
        std::ofstream fout(dirPathSafe + PakInfoFileName);
        for (const std::string &line : infoLines)
            fout << line << std::endl;
        fout.close();
    }

    void unpack_string(const std::string &outputPath, const PakItem &item)
    {
        std::string fileName = outputPath + item.name;
        if (item.type == "System.String")
            fileName += ".txt";
        else if (item.type == "System.Xml.Linq.XElement")
            fileName += ".xml";
        else
            throw std::runtime_error("wrong item type");

        std::ofstream fout;
        fout.open(fileName, std::ios::binary);
        MemoryBinaryReader reader(item.data.data());
        std::string value = reader.readString();
        fout.write(value.data(), value.length());
        fout.close();
    }

    void unpack_bitmapFont(const std::string &outputDir, const PakItem &item)
    {
        std::string listFileName = outputDir + item.name + ".lst";
        std::string textureFileName = outputDir + item.name + ".tga";

        MemoryBinaryReader reader(item.data.data());
        int glyphCount = reader.readInt32();
        std::vector<GlyphInfo> glyphList;
        glyphList.resize(glyphCount);
        for (int i = 0; i < glyphCount; ++i)
        {
            GlyphInfo &glyph = glyphList[i];
            glyph.unicode = reader.readUtf8Char();
            glyph.texCoord1.x = reader.readSingle();
            glyph.texCoord1.y = reader.readSingle();
            glyph.texCoord2.x = reader.readSingle();
            glyph.texCoord2.y = reader.readSingle();
            glyph.offset.x = reader.readSingle();
            glyph.offset.y = reader.readSingle();
            glyph.width = reader.readSingle();
        }
        float glyphHeight = reader.readSingle();
        Vector2f spacing;
        spacing.x = reader.readSingle();
        spacing.y = reader.readSingle();
        float scale = reader.readSingle();
        int fallbackCode = reader.readUtf8Char();

        int width = reader.readInt32();
        int height = reader.readInt32();
        int mipmapLevel = reader.readInt32();

        stbi_write_tga(textureFileName.c_str(), width, height, 4, item.data.data() + reader.position);

        std::ofstream fList;
        fList.open(listFileName);
        fList << glyphCount << std::endl;
        for (int i = 0; i < glyphCount; ++i)
        {
            GlyphInfo &glyph = glyphList[i];
            fList << glyph.unicode << '\t'
                << glyph.texCoord1.x << '\t' << glyph.texCoord1.y << '\t'
                << glyph.texCoord2.x << '\t' << glyph.texCoord2.y << '\t'
                << glyph.offset.x << '\t' << glyph.offset.y << '\t'
                << glyph.width << std::endl;
        }
        fList << glyphHeight << std::endl;
        fList << spacing.x << '\t' << spacing.y << std::endl;
        fList << scale << std::endl;
        fList << fallbackCode << std::endl;
        fList.close();
    }

    void unpack_soundBuffer(const std::string &outputDir, const PakItem &item)
    {
        static const int bitsPerSample = 16;

        MemoryBinaryReader reader(item.data.data());

        bool oggCompressed = reader.readBoolean();

        if (!oggCompressed)
        {
            std::string listFileName = outputDir + item.name + ".wav";
            WavHeader header;
            WavHeader::SetMagicValues(header);
            header.channelCount = reader.readInt32();
            header.sampleRate = reader.readInt32();
            header.subchunk2Size = reader.readInt32();

            header.bitsPerSample = bitsPerSample;
            header.byteRate = header.sampleRate * bitsPerSample / 8;
            header.chunkSize = header.subchunk2Size + 36;

            const byte *sound = item.data.data() + reader.position;
            std::ofstream fout(listFileName, std::ios::binary);
            fout.write(reinterpret_cast<const char*>(&header), sizeof(header));
            fout.write(reinterpret_cast<const char*>(sound), header.subchunk2Size);
            fout.close();
        }
        else
        {
            std::ofstream(outputDir + item.name, std::ios::binary).write(reinterpret_cast<const char*>(item.data.data()), item.data.size());
        }
    }
}

