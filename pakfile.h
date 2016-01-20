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
        virtual byte readByte() = 0;
        void readBytes(int size, byte buf[]);
        int readInt();
        int read7BitEncodedInt();
        std::string readString();
    private:
        std::istream *m_stream;
    };

    class StreamBinaryReader: public BinaryReader
    {
    public:
        StreamBinaryReader(std::istream *stream);

        byte readByte();
    private:
        std::istream *m_stream;
    };

    class MemoryBinaryReader: public BinaryReader
    {
    public:
        MemoryBinaryReader(const byte *buffer);
        unsigned position;

        byte readByte();
    private:
        const byte *m_buffer;
    };

    class BinaryWriter
    {
    public:
        virtual void writeByte(byte value) = 0;
        void writeBytes(int size, const byte value[]);
        void writeInt(int value);
        void write7BitEncodedInt(int value);
        void writeString(const std::string &value);
    private:
        std::ostream *m_stream;
    };

    class StreamBinaryWriter: public BinaryWriter
    {
    public:
        StreamBinaryWriter(std::ostream *stream);

        void writeByte(byte value);
    private:
        std::ostream *m_stream;
    };

    class MemoryBinaryWriter: public BinaryWriter
    {
    public:
        MemoryBinaryWriter(byte *buffer);
        unsigned position;

        void writeByte(byte value);
    private:
        byte *m_buffer;
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
        int offset = -1;
        int length = -1;
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
        const std::vector<PakItem>& contents() const;
        void addItem(const PakItem &item);
        void addItem(PakItem &&item);
        PakItem& getItem(std::size_t where);
        void deleteItem(std::size_t where);
        void removeItem(std::size_t where);
    private:
        std::vector<PakItem> m_contents;
    };
}

