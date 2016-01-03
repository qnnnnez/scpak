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
	string programName = programPath.substr(i);
	cout << "Usage: " << programName << " <directory> | <pakfile>" << endl;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printUsage(argc, argv);
		return 1;
	}
	string path = argv[1];
	if (isDirectory(path.c_str()))
	{
		PakFile pak = pack(path);
		ofstream fout(path + ".pak", ios::binary);
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
    return 0;
}
