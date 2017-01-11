#pragma once
#include <string>
#include <istream>
#include <ostream>

#include "scpak.h"


namespace scpak
{
    class BinaryReader
    {
    public:
        virtual byte readByte() = 0;
        void readBytes(int size, byte buf[]);
        int readInt();
        float readFloat();
        int read7BitEncodedInt();
        int readUtf8Char();
        bool readBoolean();
        std::string readString();

        static int getUtf8CharCount(const std::string &value);
    private:
        std::istream *m_stream;
    };

    class StreamBinaryReader : public BinaryReader
    {
    public:
        StreamBinaryReader(std::istream *stream);

        byte readByte();
    private:
        std::istream *m_stream;
    };

    class MemoryBinaryReader : public BinaryReader
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
        void writeFloat(float value);
        int write7BitEncodedInt(int value);
        int writeUtf8Char(int value);
        void writeBoolean(bool value);
        void writeString(const std::string &value);
    private:
        std::ostream *m_stream;
    };

    class StreamBinaryWriter : public BinaryWriter
    {
    public:
        StreamBinaryWriter(std::ostream *stream);

        void writeByte(byte value);
    private:
        std::ostream *m_stream;
    };

    class MemoryBinaryWriter : public BinaryWriter
    {
    public:
        MemoryBinaryWriter(byte *buffer);
        unsigned position;

        void writeByte(byte value);
    private:
        byte *m_buffer;
    };
}
