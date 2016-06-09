#include "pakfile.h"
#include "process.h"
#include "native.h"
#include <sstream>
#include <fstream>

using namespace std;
using namespace scpak;

void printUsage(int argc, char *argv[])
{
    string programPath = argv[0];
    size_t i = programPath.rfind(pathsep);
    string programName = programPath.substr(i+1);
    cout << "Usage: " << programName << " <directory> | <pakfile>" << endl;
}

int main(int argc, char *argv[])
{
    string path;
    bool interactive = false;
    if (argc != 2)
    {
        printUsage(argc, argv);
        cout << endl;
        cout << "Enter a directory to pack or a .pak file to unpack: ";
        cin >> path;
        interactive = true;
    }
    else
    {
        path = argv[1];
    }

    if (!pathExists(path.c_str()))
    {
        cerr << "error: file/directory " << path << " does not exists" << endl;
        return 1;
    }
    if (isDirectory(path.c_str()))
    {
        ofstream fout(path + ".pak", ios::binary);

        PakFile pak = pack(path);
        pak.save(fout);

        fout.close();
    }
    else if (isNormalFile(path.c_str()))
    {
        size_t i = path.rfind(".pak");
        string directoryName;
        if (i == string::npos)
            directoryName = path + "_unpack";
        else
            directoryName = path.substr(0, i);
        ifstream fin(path, ios::binary);
        PakFile pak;

        pak.load(fin);
        unpack(pak, directoryName);
    }
    if (interactive)
        cout << "Done." << endl;
    return 0;
}
