# Installation
- Needs `cmake`, `libpng`, `boost-filesystem`, `boost-locale`, and optionally, `boost-thread`

Make a folder for the build and cd into it (`mkdir build && cd build`), then `cmake ..` and finally `make`.  Disable multithreaded PNG saving with `-DENABLE_MULTITHREADING=OFF` to remove the `boost-thread` requirement.

# Usage

#### Convert one file at a time

To convert a single file, do:

`./EnterExtractor input.pic output.png`

`bup` and `txa` files contain multiple images.  To convert one, do:

`./EnterExtractor input.bup output`

This will create multiple images with names like `output_something.png`

#### Convert a folder

While the program doesn't currently have full folder support, you can use a loop in `zsh` like so:
```zsh
cd input_folder
for file in **pic; do
	mkdir -p output_folder/"$(dirname "$file")"
	EnterExtractor $file output_folder/"$(dirname "$file")"/$(basename "$file" pic)png
done
```

#### Convert a folder using our Python 3 script

You can use the `tools/enter_extractor_batch.py` script to convert a whole folder.

1. Make sure to run the script with Python 3
2. Make sure `EnterExtractor` executable is placed next to the `enter_extractor_batch.py` script, or is on your PATH.
3. Run `python enter_extractor_batch.py source_pics converted_pics`, where `source_pics` contains the `.pic` files you want to convert, and `converted_pics` is the output folder. The script will include any subdirectories in the source folder.

I've only tested this script on Windows, but let me know if you have any issues on Linux / Mac and I'll try to fix it.

# Extracting Rom Files

Use the file 'blabla.py' in this repository. The output directory will be created if it does not exist. 

`py blabla.py input.rom c:\temp\output`

Note that there is a delay between when the program starts, and when the first files start being written to disk.

# Supported Games

The C++ extractor (and `blabla.py`) is known to work with files from the following games
- Higurashi no Naku Koro Ni Sui (PS3)
- Higurashi no Naku Koro Ni Hou (Switch)
- Gensou Rougoku no Kaleidoscope (Switch, `.msk` / `MSK4` support has issues)

Other Entergram games may or may not work

# Extra Information

- An older Python version is available in `OLD_python_version`.  It may break a lot.

## Differences from Umineko format

- the header for the overall pic file, and also the header for each pic chunk is different - it has some extra bytes compared to the sui version
- the byte marked 'b1' has it's nibbles in reversed order
- the encoded image is in BGRA32 format, so a conversion is performed to output in RGBA32 format (probably done this way to suit the format the hardware natively expects)

## Other modifications from original source code

- rewrote in C++
