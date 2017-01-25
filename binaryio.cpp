#include "binaryio.h"
#include <stdexcept>

namespace scpak
{
    void BinaryReader::readBytes(int size, byte buf[])
    {
        for (int i = 0; i<size; ++i)
            buf[i] = readByte();
    }

    std::int32_t BinaryReader::readInt32()
    {
        int value;
        readBytes(4, reinterpret_cast<byte*>(&value));
        return value;
    }

    std::float_t BinaryReader::readSingle()
    {
        float value;
        readBytes(4, reinterpret_cast<byte*>(&value));
        return value;
    }

    std::int32_t BinaryReader::read7BitEncodedInt()
    {
        int value = 0;
        int offset = 0;
        while (offset != 35)
        {
            byte b = readByte();
            value |= static_cast<int>(b & 127) << offset;
            offset += 7;
            if ((b & 128) == 0)
                return value;
        }
        throw std::runtime_error("bad 7 bit encoded int32");
    }

    int BinaryReader::readUtf8Char()
    {
        byte first = readByte();
        if ((first & 0b10000000) == 0)
            // only one byte long, 0b0???????
            return static_cast<int>(first);
        if ((first & 0b11000000) == 0b11000000 && (first & 0b00100000) == 0)
        {
            // two bytes long, 0b110????? 0b10??????
            int second = readByte();
            int value = (first & 0b00011111) << 6;
            value |= second & 0b00111111;
            return value;
        }
        if ((first & 0b11100000) == 0b11100000 && (first & 0b00010000) == 0)
        {
            // three bytes long, 0b1110???? 0b10?????? 0b10??????
            int second = readByte();
            int third = readByte();
            int value = (first & 0b00001111) << 12;
            value |= (second & 0b00111111) << 6;
            value |= third & 0b00111111;
            return value;
        }
        if ((first & 0b11110000) == 0b11110000 && (first & 0b00001000) == 0)
        {
            // four bytes long, 0b11110??? 0b10?????? 0b10?????? 0b10??????
            int second = readByte();
            int third = readByte();
            int fourth = readByte();
            int value = (first & 0b00000111) << 18;
            value |= (second & 0b00111111) << 12;
            value |= (third & 0b00111111) << 6;
            value |= fourth & 0b00111111;
            return value;
        }
        throw std::runtime_error("utf-8 encoded unicode is too large");
    }

    bool BinaryReader::readBoolean()
    {
        return readByte() ? true : false;
    }

    std::string BinaryReader::readString()
    {
        int length = read7BitEncodedInt();
        std::string buffer;
        buffer.resize(length);
        for (int i = 0; i<length; ++i)
            buffer[i] = static_cast<char>(readByte());
        return buffer;
    }

    int BinaryReader::getUtf8CharCount(const std::string &value)
    {
        MemoryBinaryReader reader(reinterpret_cast<const byte*>(value.data()));
        int count = 0;
        while (reader.position < value.length())
        {
            int unicode = reader.readUtf8Char();
            ++count;
        }
        return count;
    }


    StreamBinaryReader::StreamBinaryReader(std::istream *stream)
    {
        m_stream = stream;
    }

    byte StreamBinaryReader::readByte()
    {
        if (!*m_stream)
            throw std::runtime_error("bad stream");
        char ch;
        m_stream->get(ch);
        return static_cast<byte>(ch);
    }

    MemoryBinaryReader::MemoryBinaryReader(const byte *buffer)
    {
        position = 0;
        m_buffer = buffer;
    }

    byte MemoryBinaryReader::readByte()
    {
        return m_buffer[position++];
    }

    StreamBinaryWriter::StreamBinaryWriter(std::ostream *stream)
    {
        m_stream = stream;
    }

    void StreamBinaryWriter::writeByte(byte value)
    {
        if (!m_stream)
            throw std::runtime_error("bad stream");
        char ch = static_cast<char>(value);
        m_stream->put(ch);
    }

    MemoryBinaryWriter::MemoryBinaryWriter(byte *buffer)
    {
        position = 0;
        m_buffer = buffer;
    }

    void MemoryBinaryWriter::writeByte(byte value)
    {
        m_buffer[position++] = value;
    }


    void BinaryWriter::writeBytes(int size, const byte value[])
    {
        for (int i = 0; i<size; ++i)
            writeByte(value[i]);
    }

    void BinaryWriter::writeInt(std::int32_t value)
    {
        writeBytes(4, reinterpret_cast<byte*>(&value));
    }

    void BinaryWriter::writeFloat(std::float_t value)
    {
        writeBytes(4, reinterpret_cast<byte*>(&value));
    }

    int BinaryWriter::write7BitEncodedInt(std::int32_t value)
    {
        int count = 0;
        while (value > 127)
        {
            byte n = static_cast<byte>(value & 127);
            n |= 128;
            writeByte(n); ++count;
            value >>= 7;
        }
        byte n = static_cast<byte>(value);
        writeByte(n); ++count;
        return count;
    }

    int BinaryWriter::writeUtf8Char(int value)
    {
        if (value <= 0x7f)
        {
            writeByte(static_cast<byte>(value));
            return 1;
        }
        if (value <= 0x7ff)
        {
            byte first = value >> 6;
            first &= 0b11011111;
            first |= 0b11000000;
            byte second = value & 0b00111111;
            second &= 0b10111111;
            second |= 0b10000000;
            writeByte(first);
            writeByte(second);
            return 2;
        }
        if (value <= 0xffff)
        {
            byte first = value >> 12;
            first &= 0b11101111;
            first |= 0b11100000;

            byte second = (value >> 6) & 0b00111111;
            second &= 0b10111111;
            second |= 0b10000000;

            byte third = value & 0b00111111;
            third &= 0b10111111;
            third |= 0b10000000;

            writeByte(first);
            writeByte(second);
            writeByte(third);
            return 3;
        }
        if (value <= 0x10ffff)
        {
            byte first = value >> 18;
            first &= 0b11110111;
            first |= 0b11110000;

            byte second = (value >> 12) & 0b00111111;
            second &= 0b10111111;
            second |= 0b10000000;

            byte third = (value >> 6) & 0b00111111;
            third &= 0b10111111;
            third |= 0b10000000;

            byte fourth = value & 0b00111111;
            fourth &= 0b10111111;
            fourth |= 0b10000000;

            writeByte(first);
            writeByte(second);
            writeByte(third);
            writeByte(fourth);
            return 4;
        }
        throw std::runtime_error("unicode is too large");
    }

    void BinaryWriter::writeBoolean(bool value)
    {
        writeByte(byte(value));
    }

    void BinaryWriter::writeString(const std::string &value)
    {
        write7BitEncodedInt(value.length());
        writeBytes(value.length(), reinterpret_cast<const byte*>(value.data()));
    }
}
