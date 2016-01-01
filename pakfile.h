#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <set>

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

    typedef struct
    {
        byte magic[4] = {0x50, 0x41, 0x43, 0x00};
        int fileLength;
        int contentOffset;
        int contentCount;

        bool checkMagic() const
        {
            return magic[0] == 0x50 && magic[1] == 0x41 && magic[2] == 0x43 && magic[3] == 0x00;
        }
    } PakHeader;

    typedef struct
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
        PakFile(const PakFile &) = delete;
        PakFile(PakFile &&);
        void load(std::istream &stream);
        void save(std::ostream &stream);
        const std::vector<PakItem> & contents() const;
    private:
        std::vector<PakItem> m_contents;
    };
}

