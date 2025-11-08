# PrinterFileGen
Will do one day

Some things need to be optimized (making a BMP.cpp file for example).

This was compiled using Visual Studio 2022.

File structure:
```
<project name>
  L masks
    L <display order>.<duration (sec)>.bmp
    L <display order>.<duration (sec)>.bmp
    L ...
  L preview.bmp
  L PrinterFileGen.exe
```

Example:
```
Example PCB
  L masks
    L 0.10.bmp
    L 1.80.bmp
  L preview.bmp
  L PrinterFileGen.exe
```

The .bmp mask files should be 9024x5120 pixels in black and white. White represents LCD off (UV shining through), black represents LCD on (UV blocked).
The .bmp preview file should be 224x168 pixels in any color; the .exe will automatically do color corrections to be the supported values.

The program will generate an Anycubic Photon Mono 4 .pm4n 3D printer file from the masks and the preview, as well as a temp file which can be deleted.
It likely only works on Windows at the moment.

If the masks aren't showing up correctly on the printer, load the bitmap images into mspaint and save.
There have been some issues with using Photoshop generated bitmaps that I have not yet resolved.
