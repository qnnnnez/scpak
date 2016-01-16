#include <string>
#include <stdexcept>

#if defined(__linux__)
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
namespace scpak
{
    extern const char pathsep = '/';

    void createDirectory(const char *path)
    {
        int result = mkdir(path, 0777);
        if (result == -1)
            throw std::runtime_error("failed to create directory: " + std::string(path));
    }

    bool pathExists(const char *path)
    {
        return access(path, F_OK) == 0;
    }

    int getFileSize(const char *path)
    {
        unsigned long filesize = -1;
        struct stat statbuf;
        if (stat(path, &statbuf) < 0)
            throw std::runtime_error("failed to get call stat: " + std::string(path));
        filesize = statbuf.st_size;
        return static_cast<int>(filesize);
    }

    bool isDirectory(const char *path)
    {
        struct stat statbuf;
        if (stat(path, &statbuf) < 0)
            throw std::runtime_error("failed to get call stat: " + std::string(path));
        return S_ISDIR(statbuf.st_mode);
    }

    bool isNormalFile(const char *path)
    {
        struct stat statbuf;
        if (stat(path, &statbuf) < 0)
            throw std::runtime_error("failed to get call stat: " + std::string(path));
        return S_ISREG(statbuf.st_mode);
    }
}
#elif defined(_WIN32)
# include <windows.h>
namespace scpak
{
    extern const char pathsep = '\\';

    void createDirectory(const char *path)
    {
        int result = CreateDirectoryA(path, NULL);
        if (result == 0)
            throw std::runtime_error("failed to get file size: " + std::string(path));
    }

    bool pathExists(const char *path)
    {
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind;
        hFind = FindFirstFileA(path, &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
            return false;
        else
        {
            FindClose(hFind);
            return true;
        }
    }

    int getFileSize(const char *path)
    {
        WIN32_FIND_DATA fileInfo;
        HANDLE hFind;
        hFind = FindFirstFileA(path, &fileInfo);
        if (hFind == INVALID_HANDLE_VALUE)
            throw std::runtime_error("failed to get file size: " + std::string(path));
        int size = fileInfo.nFileSizeLow;
        FindClose(hFind);
        return size;
    }

    bool isDirectory(const char *path)
    {
        DWORD attributes = GetFileAttributesA(path);
        if (attributes == INVALID_FILE_ATTRIBUTES)
            throw std::runtime_error("failed to get file attribute: " + std::string(path));
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    bool isNormalFile(const char *path)
    {
        DWORD attributes = GetFileAttributesA(path);
        if (attributes == INVALID_FILE_ATTRIBUTES)
            throw std::runtime_error("failed to get file attribute: " + std::string(path));
        return (attributes & FILE_ATTRIBUTE_NORMAL) != 0 || (attributes & FILE_ATTRIBUTE_ARCHIVE) != 0;
    }
}
#endif
