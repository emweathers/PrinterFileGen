# PrinterFileGen.exe
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

# printer_gbr_converter.py
Converts .gbr Gerber files into .bmp Bitmap image files sized at ~59 DPmm (Dots Per mm) for use with the printer.

Should serve as a half-decent pipelining tool for automating the creation of .bmp files from PCB files for use with PrinterFileGen.exe.

**Using this requires installing the libraries used in the program. It also may only work with KiCad outputs.**
**Of the output gerbers, one which ends with "Edge_Cuts.gbr" is needed to establish the PCB border.**

Instructions on Use:
- Put printer_gbr_converter.py in the same folder as .gbr files
- Run
- Output files can be found in the bmp_out folder within the folder the program was run in

TODO: Make it use Protel file extensions
