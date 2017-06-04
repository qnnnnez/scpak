#include "pakfile.h"
#include "pack.h"
#include "unpack.h"
#include "native.h"
#include <iostream>
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
    cout << "NOTE: You can just drag&drop directory or pakfile on scpak executable";
}

void printVersion()
{
	cout << "scpak version " << scpak::Version << endl;
	cout << "scpak is a tool for pack/unpack Survivalcraft pak format" << endl;
	cout << "visit https://github.com/qnnnnez/scpak for more information" << endl;
}

void printLicense()
{
	cout << "The MIT License (MIT) \nCopyright (c) 2017 qnnnnez" << endl;
}

int main(int argc, char *argv[])
{
    string path;
    bool interactive = false;
    if (argc == 1)
    {
        printUsage(argc, argv);
        cout << endl;
        cout << "Enter a directory to pack or a .pak file to unpack: ";
        cin >> path;
        interactive = true;
    }

    else if (argc == 2)
    {
    	std::string cmdarg = argv[1];
		if (cmdarg == "--help" || cmdarg == "-h")
		{
			printUsage (argc, argv);
			return 0;
		}
		else if (cmdarg == "--version" || cmdarg == "-v")
		{
			printVersion ();
			return 0;
		}
		else if (cmdarg == "--licence" || cmdarg == "--license")
		{
			printLicense ();
			return 0;
		}
		else
		{
		path = argv[1];
		}
    }
	else
	{
		cerr << "error: unrecognized command line option" <<endl;
		return 1;
	}


    if (!pathExists(path.c_str()))
    {
        cerr << "error: file/directory " << path << " does not exists" << endl;
        return 1;
    }
    if (isDirectory(path.c_str()))
    {
        ofstream fout(path + ".pak", ios::binary);

        PakFile pak = packAll(path);
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
        unpackAll(pak, directoryName);
    }
    if (interactive)
        cout << "Done." << endl;
    return 0;
}
