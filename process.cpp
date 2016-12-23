#include "process.h"
#include "native.h"
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
    const char *PakInfoFileName = "scpak.info";

    void unpack(const PakFile &pak, const std::string &dirPath, bool unpackText, bool unpackTexture, bool unpackFont)
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
                p = filePath.find('/', p+1);
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

    PakFile pack(const std::string &dirPath, bool packText, bool packTexture, bool packFont)
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
        while(std::getline(fPakInfo, line))
        {
            std::size_t split1 = line.find(':');
            if (split1 == std::string::npos)
            {
                std::stringstream ss;
                ss << "cannot parse " << PakInfoFileName << ", line " << lineNumber;
                throw std::runtime_error(ss.str());
            }
            std::string name = line.substr(0, split1);
            std::size_t split2 = line.find(':', split1+1);
            std::string type = line.substr(split1+1, split2-split1-1);
            std::string extraInfo;
            if (split2 != std::string::npos)
                extraInfo = line.substr(split2+1, std::string::npos);
            PakItem item;
            item.name = name;
            item.type = type;

            if (packText && type == "System.String")
            {
                pack_string(dirPathSafe, item);
            }
            else if (packText && type == "System.Xml.Linq.XElement")
            {
                pack_string(dirPathSafe, item);
            }
            else if (packTexture && type == "Engine.Graphics.Texture2D")
            {
                // not ready yet
                std::string filePathRaw = dirPathSafe + name;
                std::string fileName = filePathRaw;
                if (pathExists((filePathRaw + ".tga").c_str()))
                    fileName += ".tga";
                else if (pathExists((filePathRaw + ".png").c_str()))
                    fileName += ".png";
                else if (pathExists((filePathRaw + ".bmp").c_str()))
                    fileName += ".bmp";
                else if (pathExists(filePathRaw.c_str()))
                    goto pack_raw; // -_-
                else
                    throw std::runtime_error("cannot find image file: " + name);
                int width, height, comp;
                unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &comp, 0);
                if (data == nullptr)
                    throw std::runtime_error("cannot load image file: " + name);
                if (comp != 4)
                    throw std::runtime_error("image must have 4 components in every pixel: " + name);
                int mipmapLevel = 0;
                if (!extraInfo.empty())
                    std::stringstream(extraInfo) >> mipmapLevel;
                item.length = sizeof(int) * 3 + calcMipmapSize(width, height, mipmapLevel) * comp;
                item.data.resize(item.length);
                *reinterpret_cast<int*>(item.data.data()) = width;
                *(reinterpret_cast<int*>(item.data.data()) + 1) = height;
                *(reinterpret_cast<int*>(item.data.data()) + 2) = mipmapLevel;
                std::memcpy(item.data.data() + sizeof(int) * 3, data, width*height*comp);
                stbi_image_free(data);
                int offset = generateMipmap(width, height, mipmapLevel, item.data.data() + sizeof(int) * 3);
            }
            else if (packFont && type == "Engine.Media.BitmapFont")
            {
                pack_bitmapFont(dirPathSafe, item);
            }
            else
            {
            pack_raw:
                std::string filePath = dirPathSafe + name;
                int fileSize = getFileSize(filePath.c_str());
                std::ifstream file(filePath, std::ios::binary);
                item.data.resize(fileSize);
                item.length = fileSize;
                file.read(reinterpret_cast<char*>(item.data.data()), fileSize);
                file.close();
            }
            pak.addItem(std::move(item));
            ++lineNumber;
        }
        fPakInfo.close();
        return pak;
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
        int w=width, h=height;
        while (w != 1 && h%2 != 1 && level > 1)
        {
            w /= 2;
            h /= 2;
            --level;
            stbir_resize_uint8(image, width, height, 0,
                               image+offset, w, h, 0,
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

    void pack_string(const std::string &inputPath, PakItem &item)
    {
        std::string fileName = inputPath + item.name;
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

        item.data.resize(fileSize+5);
        MemoryBinaryWriter writer(item.data.data());
        writer.write7BitEncodedInt(fileSize);
        item.length = fileSize + writer.position;
        fin.read(reinterpret_cast<char*>(item.data.data()+writer.position), fileSize);
        fin.close();
    }

    void unpack_bitmapFont(const std::string &outputDir, const PakItem &item)
    {
        std::string listFileName = outputDir + item.name + ".lst";
        std::string textureFileName = outputDir + item.name + ".tga";

        MemoryBinaryReader reader(item.data.data());
        int glyphCount = reader.readInt();
        std::vector<GlyphInfo> glyphList;
        glyphList.resize(glyphCount);
        for (int i = 0; i < glyphCount; ++i)
        {
            GlyphInfo &glyph = glyphList[i];
            glyph.unicode = reader.readUtf8Char();
            glyph.texCoord1.x = reader.readFloat();
            glyph.texCoord1.y = reader.readFloat();
            glyph.texCoord2.x = reader.readFloat();
            glyph.texCoord2.y = reader.readFloat();
            glyph.offset.x = reader.readFloat();
            glyph.offset.y = reader.readFloat();
            glyph.width = reader.readFloat();
        }
        float glyphHeight = reader.readFloat();
        Vector2f spacing;
        spacing.x = reader.readFloat();
        spacing.y = reader.readFloat();
        float scale = reader.readFloat();
        int fallbackCode = reader.readUtf8Char();

        int width = reader.readInt();
        int height = reader.readInt();
        int mipmapLevel = reader.readInt();

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

    bool isPowerOfTwo(int n)
    {
        return (n & (n - 1)) == 0;
    }
}

