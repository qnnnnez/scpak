#include "process.h"
#include "native.h"
#include <stdexcept>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#if defined(_MSC_VER)
# pragma warning(disable: 4996)
# pragma warning(disable: 4244)
#endif

namespace scpak
{
    void unpack(const PakFile &pak, const std::string &dirPath)
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
            goto unpack_raw;
            if (std::strcmp(item.type, "System.String") == 0)
            {
                std::ofstream fout(dirPathSafe + item.name + ".txt", std::ios::binary);
                MemoryBinaryReader reader(item.data);
                std::string value = reader.readString();
                fout.write(value.data(), value.length());
                fout.close();
            }
            else if (std::strcmp(item.type, "System.Xml.Linq.XElement") == 0)
            {
                std::ofstream fout(dirPathSafe + item.name + ".xml", std::ios::binary);
                MemoryBinaryReader reader(item.data);
                std::string value = reader.readString();
                fout.write(value.data(), value.length());
                fout.close();
            }
			else if (std::strcmp(item.type, "Engine.Graphics.Texture2D") == 0)
			{
                goto unpack_raw;
                // not ready yet
				std::string fileName = dirPathSafe + item.name + ".tga";
				const byte *data = item.data;
				const int &width = *reinterpret_cast<const int*>(data);
				const int &height = *(reinterpret_cast<const int*>(data) + 1);
				const int &mipmapLevel = *(reinterpret_cast<const int*>(data) + 2);
				const void *imageData = reinterpret_cast<const void*>(data + sizeof(int) * 3);
				stbi_write_tga(fileName.c_str(), width, height, 4, imageData);
                
                lineBuffer << ':' << mipmapLevel;
			}
            else
            {
            unpack_raw:
                // just write out raw data
                std::ofstream fout(dirPathSafe + item.name, std::ios::binary);
                fout.write(reinterpret_cast<char*>(item.data), item.length);
                fout.close();
            }
            infoLines.push_back(lineBuffer.str());
        }
        // write pakinfo.txt - we need to repack it later
        std::ofstream fout(dirPathSafe + "pakinfo.txt");
        for (const std::string &line : infoLines)
            fout << line << std::endl;
        fout.close();
    }

    PakFile pack(const std::string &dirPath)
    {
        PakFile pak;
        std::vector<PakItem> contents = pak.contents();
        std::string dirPathSafe = dirPath;
        if (*dirPathSafe.rbegin() != pathsep)
            dirPathSafe += pathsep;
        std::ifstream fPakInfo(dirPathSafe + "pakinfo.txt");
        std::string line;
        int lineNumber = 1;
        while(std::getline(fPakInfo, line))
        {
            std::size_t split1 = line.find(':');
            if (split1 == std::string::npos)
            {
                std::stringstream ss;
                ss << "cannot parse pakinfo.txt, line " << lineNumber;
                throw std::runtime_error(ss.str());
            }
            std::string name = line.substr(0, split1);
            std::size_t split2 = line.find(':', split1+1);
            std::string type = line.substr(split1+1, split2-split1-1);
            std::string extraInfo;
            if (split2 != std::string::npos)
                extraInfo = line.substr(split2+1, std::string::npos);
            PakItem item;
            item.name = new char[name.length()+1];
            strcpy(item.name, name.c_str());
            item.type = new char[type.length()+1];
            strcpy(item.type, type.c_str());

            goto pack_raw;
            if (type == "System.String")
            {
                std::string filePath = dirPathSafe + name + ".txt";
                int fileSize = getFileSize(filePath.c_str());
                std::ifstream file(filePath, std::ios::binary);
                item.data = new byte[fileSize+5];
                MemoryBinaryWriter writer(item.data);
                writer.write7BitEncodedInt(fileSize);
                item.length = fileSize + writer.position;
                file.read(reinterpret_cast<char*>(item.data+writer.position), fileSize);
                file.close();
            }
            else if (type == "System.Xml.Linq.XElement")
            {
                std::string filePath = dirPathSafe + name + ".xml";
                int fileSize = getFileSize(filePath.c_str());
                std::ifstream file(filePath, std::ios::binary);
                item.data = new byte[fileSize+5];
                MemoryBinaryWriter writer(item.data);
                writer.write7BitEncodedInt(fileSize);
                item.length = fileSize + writer.position;
                file.read(reinterpret_cast<char*>(item.data+writer.position), fileSize);
                file.close();
            }
			else if (type == "Engine.Graphics.Texture2D")
			{
                goto pack_raw;
                // not ready yet
				std::string filePathRaw = dirPathSafe + name;
				std::string fileName = filePathRaw;
				if (pathExists((filePathRaw + ".tga").c_str()))
					fileName += ".tga";
				else if (pathExists((filePathRaw + ".png").c_str()))
					fileName += ".png";
				else if (pathExists((filePathRaw + ".bmp").c_str()))
					fileName += ".bmp";
				//else if (pathExists(filePathRaw.c_str()))
                //    goto pack_raw; // -_-
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
				item.data = new byte[item.length];
				*reinterpret_cast<int*>(item.data) = width;
				*(reinterpret_cast<int*>(item.data) + 1) = height;
				*(reinterpret_cast<int*>(item.data) + 2) = 0;
				std::memcpy(item.data + sizeof(int) * 3, data, width*height*comp);
				stbi_image_free(data);
                int offset = generateMipmap(width, height, mipmapLevel, item.data + sizeof(int) * 3);
			}
            else
            {
            pack_raw:
                std::string filePath = dirPathSafe + name;
                int fileSize = getFileSize(filePath.c_str());
                std::ifstream file(filePath, std::ios::binary);
                item.data = new byte[fileSize];
                item.length = fileSize;
                file.read(reinterpret_cast<char*>(item.data), fileSize);
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
        int size = width * height;
        while (width%2 == 0 && height%2 == 0 && level != 0)
        {
            width /= 2;
            height /= 2;
            --level;
            size += width * height;
        }
        return size;
    }

    int generateMipmap(int width, int height, int level, unsigned char *image)
    {
        const int comp = 4;
        int offset = width * height * comp;
        int w=width, h=height;
        while (w%2 == 0 && h%2 == 0 && level != 0)
        {
            w /= 2;
            h /= 2;
            stbir_resize_uint8(image, width, height, 0,
                               image+offset, w, h, 0,
                               comp);
            offset += w * h * comp;
            --level;
        }
        return offset;
    }
}

