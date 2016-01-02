#include "pakfile.h"
#include "process.h"
#include <sstream>
#include <fstream>

using namespace std;
using namespace scpak;

int main(int argc, char *argv[])
{
    PakFile pak = pack("./Content");
    ofstream fout("Content2.pak");
    pak.save(fout);
    fout.close();
    return 0;
}

