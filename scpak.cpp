#include "pakfile.h"
#include <sstream>

using namespace std;
using namespace scpak;

int main()
{
    stringstream ss;
    BinaryWriter writer(&ss);
    writer.writeString("test test test test test");
    BinaryReader reader(&ss);
    string str = reader.readString();
    cout << str << endl;
}

