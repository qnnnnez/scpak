#pragma once

#include <fstream>
#include <vector>

#include "scpak.h"
#include "binaryio.h"

namespace scpak
{
    class BadPakException : public BaseException
    {
    public:
        BadPakException(const char *what);
        virtual const char * what() const;

    private:
        std::string what_;
    };


    typedef struct // PakHeader
    {
        byte magic[4] = { byte('P'), byte('A'), byte('K'), byte('\0') };
        std::int32_t fileLength;
        std::int32_t contentOffset;
        std::int32_t contentCount;

        bool checkMagic() const
        {
            return magic[0] == byte('P') && magic[1] == byte('A') && magic[2] == byte('K') && magic[3] == byte('\0');
        }
    } PakHeader;

    typedef struct // PakItem
    {
        std::string name;
        std::string type;
        int offset = -1;
        int length = -1;
        std::vector<byte> data;
    } PakItem;

    class PakFile
    {
    public:
        void load(std::istream &stream);
        void save(std::ostream &stream);
        const std::vector<PakItem>& contents() const;
        void addItem(const PakItem &item);
        void addItem(PakItem &&item);
        PakItem& getItem(std::size_t where);
        void removeItem(std::size_t where);
    private:
        std::vector<PakItem> m_contents;
    };
}

