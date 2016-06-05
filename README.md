# scpak
A packer/unpacker of Content.pak from [Survivalcraft](https://kaalus.wordpress.com/).

## Build
Use cmake to build a binary. Remember to do a ```git submodule init``` before building.

## Usage
### To Unpack a Content.pak File:
```scpak.exe Content.pak```

After this operation, a directory named Content will be created, containing all unpacked files in the Content.pak.
Now that the unpacking is finished, you are free to modify unpacked files.
### To Pack a Unpacked Content Directory:
```scpak.exe Content```

After this operation, you will find a repacked Content.pak.
Replace the original Content.pak with the new one and try it out!
