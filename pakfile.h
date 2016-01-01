#pragma once

#include <iostream>
#include <fstream>
#include <vector>

namespace scpak
{
    typedef unsigned char byte;

    class BinaryReader
    {
    public:
        BinaryReader(std::istream *stream);
        byte readByte();
        void readBytes(int size, byte buf[]);
        int readInt();
        int read7BitEncodedInt();
        std::string readString();
    private:
        std::istream *m_stream;
    };

    class BinaryWriter
    {
    public:
        BinaryWriter(std::ostream *stream);
        void writeByte(byte value);
        void writeBytes(int size, const byte value[]);
        void writeInt(int value);
        void write7BitEncodedInt(int value);
        void writeString(const std::string &value);
    private:
        std::ostream *m_stream;
    };

    typedef struct // PakHeader
    {
        byte magic[4] = {byte('P'), byte('A'), byte('K'), byte('\0')};
        int fileLength;
        int contentOffset;
        int contentCount;

        bool checkMagic() const
        {
            return magic[0] == byte('P') && magic[1] == byte('A') && magic[2] == byte('K') && magic[3] == byte('\0');
        }
    } PakHeader;

    typedef struct // PakItem
    {
        char *name = nullptr;
        char *type = nullptr;
        int offset;
        int length;
        byte *data = nullptr;
    } PakItem;

    class PakFile
    {
    public:
        PakFile();
        ~PakFile();
        PakFile(const PakFile &) = delete;
        PakFile(PakFile &&);
        void load(std::istream &stream);
        void save(std::ostream &stream);
        const std::vector<PakItem> & contents() const;
    private:
        std::vector<PakItem> m_contents;
    };
}

