#include "pakfile.h"

#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <cstring>

namespace scpak
{
    BinaryReader::BinaryReader(std::istream *stream)
    {
        m_stream = stream;
    }

    byte BinaryReader::readByte()
    {
        char ch;
        m_stream->get(ch);
        return static_cast<byte>(ch);
    }

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

    std::string BinaryReader::readString()
    {
        int length = read7BitEncodedInt();
        char *buf = new char[length];
        m_stream->read(buf, length);
        std::string result(buf, buf + length);
        delete[] buf;
        return result;
    }

    BinaryWriter::BinaryWriter(std::ostream *stream)
    {
        m_stream = stream;
    }

    void BinaryWriter::writeByte(byte value)
    {
        char data = static_cast<char>(value);
        m_stream->put(data);
    }

    void BinaryWriter::writeBytes(int size, const byte value[])
    {
        for (int i=0; i<size; ++i)
        {
            char data = static_cast<char>(value[i]);
            m_stream->put(data);
        }
    }

    void BinaryWriter::writeInt(int value)
    {
        writeBytes(4, reinterpret_cast<byte*>(&value));
    }


    void BinaryWriter::write7BitEncodedInt(int value)
    {
        while ((value & 128) != 0)
        {
            byte n = static_cast<byte>(value & 127);
            n |= 128;
            writeByte(n);
            value >>= 7;
        }
        byte n = static_cast<byte>(value);
        writeByte(n);
    }

    void BinaryWriter::writeString(const std::string &value)
    {
        write7BitEncodedInt(value.length());
        // std::ostream_iterator<std::string::value_type> it(*m_stream);
        // std::copy(value.begin(), value.end(), it);
        m_stream->write(value.data(), value.length());
    }

    PakFile::PakFile()
    {
    }

    PakFile::PakFile(PakFile &&other):
        m_contents(std::move(other.m_contents))
    {
    }

    PakFile::~PakFile()
    {
        for (const PakItem &item : m_contents)
        {
            delete[] item.name;
            delete[] item.type;
            delete[] item.data;
        }
    }

    void PakFile::load(std::istream &stream)
    {
        // read header
        PakHeader header;
        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!header.checkMagic())
            throw std::runtime_error("bad pak file");
        BinaryReader reader(&stream);
        // read content dictionary
        for (int i=0; i<header.contentCount; ++i)
        {
            std::string name = reader.readString();
            std::string type = reader.readString();
            int offset = reader.readInt();
            int length = reader.readInt();
            PakItem item;
            item.name = new char[name.length()+1];
            std::strcpy(item.name, name.c_str());
            item.type = new char[type.length()+1];
            std::strcpy(item.type, type.c_str());
            item.offset = offset;
            item.length = length;
            m_contents.push_back(item);
        }
        // read all contents
        for (PakItem &item : m_contents)
        {
            stream.seekg(header.contentOffset + item.offset, std::ios::beg);
            item.data = new byte[item.length];
            stream.read(reinterpret_cast<char*>(item.data), item.length);
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
        BinaryWriter writer(&stream);
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
            item.offset = stream.tellp();
            stream.write(reinterpret_cast<char*>(item.data), item.length);
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
        PakItem newItem;
        newItem.name = new char[strlen(item.name)+1];
        strcpy(newItem.name, item.name);
        newItem.type = new char[strlen(item.type)+1];
        strcpy(newItem.type, item.type);
        newItem.length = item.length;
        newItem.data = new byte[newItem.length];
        memcpy(newItem.data, item.data, newItem.length);
        m_contents.push_back(newItem);
    }

    void PakFile::addItem(PakItem &&item)
    {
        m_contents.push_back(item);
        item.name = nullptr;
        item.type = nullptr;
        item.data = nullptr;
    }

    PakItem& PakFile::getItem(std::size_t where)
    {
        return m_contents.at(where);
    }

    void PakFile::removeItem(std::size_t where)
    {
        m_contents.erase(m_contents.begin()+where);
    }

    void PakFile::deleteItem(std::size_t where)
    {
        PakItem &item = m_contents.at(where);
        delete[] item.name;
        delete[] item.type;
        delete[] item.data;
        m_contents.erase(m_contents.begin()+where);
    }
}

