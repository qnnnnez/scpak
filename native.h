#pragma once

namespace scpak
{
    extern const char pathsep;
    void createDirectory(const char *path);
    bool pathExists(const char *path);
    int getFileSize(const char *path);
    bool isDirectory(const char *path);
    bool isNormalFile(const char *path);
}
