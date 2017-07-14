#include "pakfile.h"

#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <cstring>


namespace scpak
{
    BadPakException::BadPakException(const char *what) :
        BaseException(), what_(what)
    { }

    const char * BadPakException::what() const
    {
        return what_.c_str();
    }


    void PakFile::load(std::istream &stream)
    {
        // read header
        PakHeader header;
        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!header.checkMagic())
            throw BadPakException("invalid pak header");
        StreamBinaryReader reader(&stream);
        // read content dictionary
        for (int i = 0; i<header.contentCount; ++i)
        {
            PakItem item;
            item.name = reader.readString();
            item.type = reader.readString();
            item.offset = reader.readInt32();
            item.length = reader.readInt32();
            addItem(std::move(item));
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
            // there is a magic number before every content data in origin Content.pak
            // it's DEADBEEF
            stream.put(0xDE);
            stream.put(0xAD);
            stream.put(0xBE);
            stream.put(0xEF);
            item.offset = static_cast<int>(stream.tellp()) - header.contentOffset;
            stream.write(reinterpret_cast<char*>(item.data.data()), item.length);
        }
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
        m_contents.erase(m_contents.begin() + where);
    }
}

