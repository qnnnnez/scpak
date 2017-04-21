#include "pack.h"
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
    int calcMipmapSize(int width, int height, int level = 0);
    int generateMipmap(int width, int height, int level, unsigned char *image);

    template<void old_packer(const std::string &inputDir, PakItem &output)>
    void packer_wrapper(const std::string &inputDir, PakItem &output, const std::string &meta)
    {
        old_packer(inputDir, output);
    }


    PakFile pack(const std::string &dirPath, const std::map<std::string, packer_type> &packers, const packer_type &default_packer)
    {
        PakFile pak;
        std::vector<PakItem> contents = pak.contents();
        std::string dirPathSafe = dirPath;
        if (*dirPathSafe.rbegin() != pathsep)
            dirPathSafe += pathsep;
        std::ifstream fPakInfo(dirPathSafe + PakInfoFileName);
        if (!fPakInfo)
            throw std::runtime_error("cannot open " + dirPathSafe + PakInfoFileName);
        std::string line;
        int lineNumber = 1;
        while (std::getline(fPakInfo, line))
        {
            std::size_t split1 = line.find(':');
            if (split1 == std::string::npos)
            {
                std::stringstream ss;
                ss << "cannot parse " << PakInfoFileName << ", line " << lineNumber;
                throw std::runtime_error(ss.str());
            }
            std::string name = line.substr(0, split1);
            std::size_t split2 = line.find(':', split1 + 1);
            std::string type = line.substr(split1 + 1, split2 - split1 - 1);
            std::string extraInfo;
            if (split2 != std::string::npos)
                extraInfo = line.substr(split2 + 1, std::string::npos);
            PakItem item;
            item.name = name;
            item.type = type;

            auto it = packers.find(item.type);
            if (it != packers.end())
                it->second(dirPathSafe, item, extraInfo);
            else
                default_packer(dirPathSafe, item, extraInfo);

            pak.addItem(std::move(item));
            ++lineNumber;
        }
        fPakInfo.close();
        return pak;
    }

    PakFile pack(const std::string &dirPath, bool packText, bool packTexture, bool packFont, bool packSound)
    {
        std::map<std::string, packer_type> packers;
        if (packText)
        {
            packers.insert(std::pair<std::string, packer_type>("System.String", packer_wrapper<pack_string>));
            packers.insert(std::pair<std::string, packer_type>("System.Xml.Linq.XElement", packer_wrapper<pack_string>));
        }
        if (packTexture)
            packers.insert(std::pair<std::string, packer_type>("Engine.Graphics.Texture2D", pack_texture));
        if (packFont)
            packers.insert(std::pair<std::string, packer_type>("Engine.Media.BitmapFont", packer_wrapper<pack_bitmapFont>));
        if (packSound)
            packers.insert(std::pair<std::string, packer_type>("Engine.Audio.SoundBuffer", packer_wrapper<pack_soundBuffer>));

        packer_type default_packer = packer_wrapper<pack_raw>;
        return pack(dirPath, packers, default_packer);
    }

    PakFile packAll(const std::string & dirPath)
    {
        return pack(dirPath, true, true, true, true);
    }
    

    void pack_raw(const std::string & inputDir, PakItem & item)
    {
        std::string filePath = inputDir + item.name;
        int fileSize = getFileSize(filePath.c_str());
        std::ifstream file(filePath, std::ios::binary);
        item.data.resize(fileSize);
        item.length = fileSize;
        file.read(reinterpret_cast<char*>(item.data.data()), fileSize);
    }

    void pack_string(const std::string &inputDir, PakItem &item)
    {
        std::string fileName = inputDir + item.name;
        if (item.type == "System.String")
            fileName += ".txt";
        else if (item.type == "System.Xml.Linq.XElement")
            fileName += ".xml";
        else
            throw std::runtime_error("wrong item type");

        std::ifstream fin;
        fin.open(fileName, std::ios::binary);
        fin.seekg(0, std::ios::beg);
        int offsetBeg = fin.tellg();
        fin.seekg(0, std::ios::end);
        int offsetEnd = fin.tellg();
        int fileSize = offsetEnd - offsetBeg;
        fin.seekg(0, std::ios::beg);

        item.data.resize(fileSize + 5);
        MemoryBinaryWriter writer(item.data.data());
        writer.write7BitEncodedInt(fileSize);
        item.length = fileSize + writer.position;
        fin.read(reinterpret_cast<char*>(item.data.data() + writer.position), fileSize);
        fin.close();
    }

    void pack_bitmapFont(const std::string &inputDir, PakItem &item)
    {
        std::string listFileName = inputDir + item.name + ".lst";
        std::string textureFileName = inputDir + item.name + ".tga";

        std::ifstream fList;
        fList.open(listFileName);
        int glyphCount;
        fList >> glyphCount;

        int width, height, comp;
        unsigned char *data = stbi_load(textureFileName.c_str(), &width, &height, &comp, 4);
        item.data.resize(sizeof(GlyphInfo) * glyphCount + 50 + width*height * 4);

        MemoryBinaryWriter writer(item.data.data());
        writer.writeInt(glyphCount);
        for (int i = 0; i < glyphCount; ++i)
        {
            GlyphInfo glyph;
            fList >> glyph.unicode
                >> glyph.texCoord1.x >> glyph.texCoord1.y
                >> glyph.texCoord2.x >> glyph.texCoord2.y
                >> glyph.offset.x >> glyph.offset.y
                >> glyph.width;
            writer.writeUtf8Char(glyph.unicode);
            writer.writeFloat(glyph.texCoord1.x);
            writer.writeFloat(glyph.texCoord1.y);
            writer.writeFloat(glyph.texCoord2.x);
            writer.writeFloat(glyph.texCoord2.y);
            writer.writeFloat(glyph.offset.x);
            writer.writeFloat(glyph.offset.y);
            writer.writeFloat(glyph.width);
        }
        float glyphHeight; fList >> glyphHeight;
        Vector2f spacing; fList >> spacing.x >> spacing.y;
        float scale; fList >> scale;
        int fallbackCode; fList >> fallbackCode;
        writer.writeFloat(glyphHeight);
        writer.writeFloat(spacing.x);
        writer.writeFloat(spacing.y);
        writer.writeFloat(scale);
        writer.writeUtf8Char(fallbackCode);
        fList.close();

        writer.writeInt(width);
        writer.writeInt(height);
        writer.writeInt(1);
        std::memcpy(item.data.data() + writer.position, data, width*height * 4);
        item.length = writer.position + width*height * 4;

        stbi_image_free(data);
    }

    void pack_texture(const std::string & inputDir, PakItem & item, const std::string &meta)
    {
        std::string filePathRaw = inputDir + item.name;
        std::string fileName = filePathRaw;
        if (pathExists((filePathRaw + ".tga").c_str()))
            fileName += ".tga";
        else if (pathExists((filePathRaw + ".png").c_str()))
            fileName += ".png";
        else if (pathExists((filePathRaw + ".bmp").c_str()))
            fileName += ".bmp";
        else if (pathExists(filePathRaw.c_str()))
        {
            pack_raw(inputDir, item);
            return;
        }
        else
            throw std::runtime_error("cannot find image file: " + item.name);
        int width, height, comp;
        unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &comp, 0);
        if (data == nullptr)
            throw std::runtime_error("cannot load image file: " + item.name);
        if (comp != 4)
            throw std::runtime_error("image must have 4 components in every pixel: " + item.name);
        int mipmapLevel = 0;
        if (!meta.empty())
            mipmapLevel = std::stoi(meta);
        item.length = sizeof(int) * 3 + calcMipmapSize(width, height, mipmapLevel) * comp;
        item.data.resize(item.length);
        *reinterpret_cast<int*>(item.data.data()) = width;
        *(reinterpret_cast<int*>(item.data.data()) + 1) = height;
        *(reinterpret_cast<int*>(item.data.data()) + 2) = mipmapLevel;
        std::memcpy(item.data.data() + sizeof(int) * 3, data, width*height*comp);
        stbi_image_free(data);
        int offset = generateMipmap(width, height, mipmapLevel, item.data.data() + sizeof(int) * 3);
    }

    void pack_soundBuffer(const std::string &inputDir, PakItem &item)
    {
        std::string inputFilePathBase = inputDir + item.name;
        if (pathExists(inputFilePathBase.c_str()))
        {
            int fileSize = getFileSize(inputFilePathBase.c_str());
            std::ifstream file(inputFilePathBase, std::ios::binary);
            item.data.resize(fileSize);
            item.length = fileSize;
            file.read(reinterpret_cast<char*>(item.data.data()), fileSize);
            file.close();
        }
        else if (pathExists((inputFilePathBase + ".wav").c_str()))
        {
            static const int bitsPerSample = 16;

            std::string fileName = inputFilePathBase + ".wav";
            std::ifstream fin(fileName, std::ios::binary);
            WavHeader header;
            fin.read(reinterpret_cast<char*>(&header), sizeof(header));
            item.data.resize(header.subchunk2Size + 13);
            MemoryBinaryWriter writer(item.data.data());
            writer.writeBoolean(false);
            writer.writeInt(header.channelCount);
            writer.writeInt(header.sampleRate);
            writer.writeInt(header.subchunk2Size);

            if (header.bitsPerSample != 16)
            {
                throw std::runtime_error("WAV-" + fileName + ": bitsPerSample must be 16.");
            }

            fin.read(reinterpret_cast<char*>(item.data.data() + writer.position), header.subchunk2Size);
            fin.close();
            item.length = item.data.size();
        }
    }


    int calcMipmapSize(int width, int height, int level)
    {
        if (level == 1)
            return width * height;
        if (!isPowerOfTwo(width) || !isPowerOfTwo(height))
            throw std::runtime_error("generating mipmaps for non-power of 2 images not supported");

        int size = width * height;
        while (width != 1 && height != 1 && level > 1)
        {
            width /= 2;
            height /= 2;
            --level;
            size += width * height;
        }
        if (width == 1 && height != 1)
            while (height != 1 && level > 1)
            {
                height /= 2;
                --level;
                size += width * height;
            }
        else if (height == 1 && width != 1)
            while (width != 1 && level > 1)
            {
                width /= 2;
                --level;
                size += width*height;
            }
        return size;
    }

    int generateMipmap(int width, int height, int level, unsigned char *image)
    {
        const int comp = 4;

        if (level == 1)
            return width * height * comp;
        if (!isPowerOfTwo(width) || !isPowerOfTwo(height))
            throw std::runtime_error("generating mipmaps for non-power of 2 images not supported");

        int offset = width * height * comp;
        int w = width, h = height;
        while (w != 1 && h % 2 != 1 && level > 1)
        {
            w /= 2;
            h /= 2;
            --level;
            stbir_resize_uint8(image, width, height, 0,
                image + offset, w, h, 0,
                comp);
            offset += w * h * comp;
        }
        if (w == 1 && h != 1)
            while (h != 1 && level > 1)
            {
                h /= 2;
                --level;
                stbir_resize_uint8(image, width, height, 0,
                    image + offset, w, h, 0,
                    comp);
                offset += w * h * comp;
            }
        else if (h == 1 && w != 1)
            while (w != 1 && level > 1)
            {
                w /= 2;
                --level;
                stbir_resize_uint8(image, width, height, 0,
                    image + offset, w, h, 0,
                    comp);
                offset += w * h * comp;
            }
        return offset;
    }
}

