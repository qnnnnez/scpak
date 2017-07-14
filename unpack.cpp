#include "unpack.h"
#include "native.h"
#include "wav.h"
#include <stdexcept>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"


namespace scpak
{
    void unpack(const PakFile &pak, const std::string &dirPath,
        const std::map<std::string, unpacker_type> &unpackers,
        const unpacker_type &default_unpacker)
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
            auto it = unpackers.find(itemType);
            std::string meta;
            if (it != unpackers.end())
                meta = it->second(dirPathSafe, item);
            else
                meta = default_unpacker(dirPathSafe, item);
            lineBuffer << ':' << meta;
            infoLines.push_back(lineBuffer.str());
        }
        // write info file - will be useful when re-packing
        std::ofstream fout(dirPathSafe + PakInfoFileName);
        for (const std::string &line : infoLines)
            fout << line << std::endl;
    }

    template<void old_unpacker(const std::string &outputPath, const PakItem &item)>
    std::string unpacker_wrapper(const std::string &outputDir, const PakItem &item)
    {
        old_unpacker(outputDir, item);
        return "";
    }

    void unpack(const PakFile &pak, const std::string &dirPath, bool unpackText, bool unpackBitmapFont, bool unpackTexture, bool unpackSound)
    {
        std::map<std::string, unpacker_type> unpackers;
        if (unpackText)
        {
            unpackers.insert(std::pair<std::string, unpacker_type>("System.String", unpacker_wrapper<unpack_string>));
            unpackers.insert(std::pair<std::string, unpacker_type>("System.Xml.Linq.XElement", unpacker_wrapper<unpack_string>));
        }
        if (unpackTexture)
            unpackers.insert(std::pair<std::string, unpacker_type>("Engine.Graphics.Texture2D", unpack_texture));
        if (unpackBitmapFont)
            unpackers.insert(std::pair<std::string, unpacker_type>("Engine.Media.BitmapFont", unpacker_wrapper<unpack_bitmapFont>));
        if (unpackSound)
            unpackers.insert(std::pair<std::string, unpacker_type>("Engine.Audio.SoundBuffer", unpacker_wrapper<unpack_soundBuffer>));
        
        unpacker_type defaultUnpacker = unpacker_wrapper<unpack_raw>;
        unpack(pak, dirPath, unpackers, defaultUnpacker);
    }

    void unpackAll(const PakFile & pak, const std::string & dirPath)
    {
        unpack(pak, dirPath, true, true, true, true);
    }

    
    void unpack_raw(const std::string &outputPath, const PakItem &item)
    {
        std::ofstream fout(outputPath + item.name, std::ios::binary);
        fout.write(reinterpret_cast<const char*>(item.data.data()), item.length);
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

        bool keepSourceImageInTag = reader.readBoolean();
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
    }

    std::string unpack_texture(const std::string &outputDir, const PakItem &item)
    {
        std::string fileName = outputDir + item.name + ".tga";
        MemoryBinaryReader reader(item.data.data());
        bool keepSourceImageInTag = reader.readBoolean();
        int width = reader.readInt32();
        int height = reader.readInt32();
        int mipmapLevel = reader.readInt32();
        const void *imageData = reinterpret_cast<const void*>(item.data.data() + reader.position);
        stbi_write_tga(fileName.c_str(), width, height, 4, imageData);

        std::string meta;
        meta += keepSourceImageInTag ? '1' : '0';
        meta += ' ';
        meta += std::to_string(mipmapLevel);
        return meta;
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
        }
        else
        {
            std::ofstream(outputDir + item.name, std::ios::binary).write(reinterpret_cast<const char*>(item.data.data()), item.data.size());
        }
    }
}

