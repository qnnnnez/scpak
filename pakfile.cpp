#include "pakfile.h"

#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <cstring>


namespace scpak
{
    void BinaryReader::readBytes(int size, byte buf[])
    {
        for (int i=0; i<size; ++i)
            buf[i] = readByte();
    }

    int BinaryReader::readInt()
    {
        int value;
        readBytes(4, reinterpret_cast<byte*>(&value));
        return value;
    }

    float BinaryReader::readFloat()
    {
        float value;
        readBytes(4, reinterpret_cast<byte*>(&value));
        return value;
    }

    int BinaryReader::read7BitEncodedInt()
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
        if ( (first & 0b11000000) == 0b11000000 && (first & 0b00100000) == 0 )
        {
            // two bytes long, 0b110????? 0b10??????
            int second = readByte();
            int value = (first & 0b00011111) << 6;
            value |= second & 0b00111111;
            return value;
        }
        if ( (first & 0b11100000) == 0b11100000 && (first & 0b00010000) == 0)
        {
            // three bytes long, 0b1110???? 0b10?????? 0b10??????
            int second = readByte();
            int third = readByte();
            int value = (first & 0b00001111) << 12;
            value |= (second & 0b00111111) << 6;
            value |= third & 0b00111111;
            return value;
        }
        if ( (first & 0b11110000) == 0b11110000 && (first & 0b00001000) == 0)
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

    std::string BinaryReader::readString()
    {
        int length = read7BitEncodedInt();
        std::string buffer;
        buffer.resize(length);
        for (int i=0; i<length; ++i)
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
        for (int i=0; i<size; ++i)
            writeByte(value[i]);
    }

    void BinaryWriter::writeInt(int value)
    {
        writeBytes(4, reinterpret_cast<byte*>(&value));
    }

    void BinaryWriter::writeFloat(float value)
    {
        writeBytes(4, reinterpret_cast<byte*>(&value));
    }

    int BinaryWriter::write7BitEncodedInt(int value)
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

            byte third = (value >> 6) &0b00111111;
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

    void BinaryWriter::writeString(const std::string &value)
    {
        write7BitEncodedInt(value.length());
        writeBytes(value.length(), reinterpret_cast<const byte*>(value.data()));
    }


    void PakFile::load(std::istream &stream)
    {
        // read header
        PakHeader header;
        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!header.checkMagic())
            throw std::runtime_error("bad pak file");
        StreamBinaryReader reader(&stream);
        // read content dictionary
        for (int i=0; i<header.contentCount; ++i)
        {
            PakItem item;
            item.name = reader.readString();
            item.type = reader.readString();
            item.offset = reader.readInt();
            item.length = reader.readInt();
            m_contents.push_back(item);
        }
        // read all contents
        for (PakItem &item : m_contents)
        {
            stream.seekg(header.contentOffset + item.offset, std::ios::beg);
            item.data.resize(item.length);
            stream.read(reinterpret_cast<char*>(item.data.data()), item.length);
            item.offset = -1; // we will not be able to access the stream
        }
    }

    void PakFile::save(std::ostream &stream)
    {
        // we cannot detect the length of the file and where content begins,
        // so we have to write the header twice
        // write file header for the first time
        PakHeader header;
        header.contentCount = m_contents.size();
        StreamBinaryWriter writer(&stream);
        stream.write(reinterpret_cast<char*>(&header), sizeof(header));
        // we do not know where the content will locate, so it is also
        // necessary to write content dictionary twice
        // write content dictionary for the first time
        for (const PakItem &item : m_contents)
        {
            writer.writeString(item.name);
            writer.writeString(item.type);
            writer.writeInt(item.offset); // should be -1
            writer.writeInt(item.length);
        }
        header.contentOffset = stream.tellp();
        // write content items
        for (PakItem &item : m_contents)
        {
            // there are a magic number before every content data in origin Content.pak
            // it's DEADBEEF
            stream.put(0xDE);
            stream.put(0xAD);
            stream.put(0xBE);
            stream.put(0xEF);
            item.offset = static_cast<int>(stream.tellp()) - header.contentOffset;
            stream.write(reinterpret_cast<char*>(item.data.data()), item.length);
        }

        header.fileLength = stream.tellp();
        // write the header again
        stream.seekp(0, std::ios::beg);
        stream.write(reinterpret_cast<char*>(&header), sizeof(header));
        // write content dictionary again
        for (PakItem &item : m_contents)
        {
            writer.writeString(item.name);
            writer.writeString(item.type);
            writer.writeInt(item.offset);
            writer.writeInt(item.length);
            item.offset = -1; // as items can be modified, it is meaningless to keep a offset
        }
    }

    const std::vector<PakItem>& PakFile::contents() const
    {
        return m_contents;
    }

    void PakFile::addItem(const PakItem &item)
    {
        m_contents.push_back(item);
    }

    void PakFile::addItem(PakItem &&item)
    {
        m_contents.push_back(std::move(item));
    }

    PakItem& PakFile::getItem(std::size_t where)
    {
        return m_contents.at(where);
    }

    void PakFile::removeItem(std::size_t where)
    {
        m_contents.erase(m_contents.begin()+where);
    }
}

