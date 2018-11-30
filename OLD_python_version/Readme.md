# Installation
- Needs Python 3 installed
- Needs Bitstring (Run `pip install bitstring`)
- Needs PyQt5 (Run `pip install pyqt5`. Ignore any IDE warnings - this is because of pyqt5's wierd way of installing)

# Usage

#### Convert a folder recursively

To convert a folder (currently pic files only), do:

`py main.py "c:\example\input_folder" "c:\example\output_folder"`

Any non pic files will be ignored, and folder structure will be preserved.

#### Convert one file at a time

To convert a single file, do:

`py convert_one.py input.pic output.png`

The output file format MUST be .png or else the program will silently fail!

If a file is not a .pic file, it will be treated as a .bup file

# Known Problems

- On some files you'll get alot of `QImage::pixel: coordinate (12,34)` out of range and `QImage::setPixel: coordinate (14,34)` out of range errors - just ignore them.
- Some images (transparent images?) appear to have corrupt chunks. I'm not sure why.
- You can't decode umi-format images. Use ps3umi for that.

# Extracting Rom Files

Use the file 'blabla.py' in this repository. The output directory will be created if it does not exist. 

`py blabla.py input.rom c:\temp\output`

Note that there is a delay between when the program starts, and when the first files start being written to disk.

# Extra Information

## Differences from Umineko format

- the header for the overall pic file, and also the header for each pic chunk is different - it has some extra bytes compared to the sui version
- the byte marked 'b1' has it's nibbles in reversed order
- the encoded image is in BGRA32 format, so a conversion is performed to output in RGBA32 format (probably done this way to suit the format the hardware natively expects)

## Other modifications from original source code

- updated code for python 3 instead of 2
- use PyQt5 instead of PyQt4