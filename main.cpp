#include "pakfile.h"
#include "process.h"
#include <sstream>
#include <fstream>

using namespace std;
using namespace scpak;

int main(int argc, char *argv[])
{
	ifstream fin("Content.pak", ios::binary);
	PakFile pak;
	pak.load(fin);
	fin.close();
	unpack(pak, "./Content");
    return 0;
}

