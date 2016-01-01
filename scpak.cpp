#include "pakfile.h"
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
    auto items = pak.contents();
    for(auto item : items)
    {
        cout << item.name << endl;
    }
    ofstream fout("Content2.pak", ios::binary);
    stringstream ss(reinterpret_cast<char*>(items[0].data));
    BinaryReader reader(&ss);
    cout << reader.readString() << endl;
    return 0;
}

