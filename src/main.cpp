#include <stdint.h>

#include <iostream>
#include <fstream>

#include <boost/endian/buffers.hpp>

#include "Config.hpp"
#include "FileTypes.hpp"

int usage(int argc, const char **argv) {
	std::cerr << "Usage: " << argv[0] << " file.(pic|bup|txa|msk) file.png OPTIONS" << std::endl;
	std::cerr << "    Converts Switch and PS3 Higurashi picture file file.pic to PNG file.png" << std::endl;
	std::cerr << "Options:" << std::endl;
	std::cerr << "    -replace replacement.png: Convert the given png to a file of the same type as the input and write it to the output" << std::endl;
	std::cerr << "    -debug-images debugImagesFolder: Write individual chunks to the given folder for debugging" << std::endl;
	std::cerr << "    -bup-parts: Output separately combinable parts instead of precombined images when decoding bup files" << std::endl;
	exit(1);
}

fs::path currentFileName;
bool SHOULD_WRITE_DEBUG_IMAGES = false;
bool SAVE_BUP_AS_PARTS = false;
fs::path debugImagePath;

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shellapi.h>
#endif

int main(int argc, const char **argv) {
	fs::path inFilename, outFilename, replace;
#ifdef _WIN32
	int wargc;
	LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
	LPWSTR* arg_fnames = wargv;
#else
	const char** arg_fnames = argv;
#endif
	for (int i = 1; i < argc; i++) {
		if (0 == strcmp(argv[i], "-bup-parts")) {
			SAVE_BUP_AS_PARTS = true;
		}
		else if (0 == strcmp(argv[i], "-debug-images")) {
			i++;
			if (i >= argc) {
				usage(argc, argv);
			}
			debugImagePath = arg_fnames[i];
			SHOULD_WRITE_DEBUG_IMAGES = true;
		}
		else if (0 == strcmp(argv[i], "-replace")) {
			i++;
			if (i >= argc) {
				usage(argc, argv);
			}
			replace = arg_fnames[i];
		}
		else if (inFilename.empty()) {
			inFilename = arg_fnames[i];
		}
		else if (outFilename.empty()) {
			outFilename = arg_fnames[i];
		}
		else {
			usage(argc, argv);
		}
	}
	if (inFilename.empty() || outFilename.empty()) {
		usage(argc,argv);
	}
	currentFileName = inFilename;

	std::ifstream in(argv[1], std::ios::binary);
	if (!in) {
		std::cerr << "Failed to open file " << argv[1] << std::endl;
		exit(EXIT_FAILURE);
	}

	boost::endian::big_int32_buf_t magic;
	
	in.read((char *)&magic, 4);
	in.seekg(0, in.beg);

	if (!replace.empty()) {
		fs::ofstream outfile(outFilename, std::ios::binary);
		switch (magic.value()) {
			case 'PIC4': return replacePic(in, outfile, replace);
			case 'TXA4': return replaceTxa(in, outfile, replace);
		}
		char *chars = (char *)&magic;
		std::cerr << argv[1] << ": file type '" << chars[0] << chars[1] << chars[2] << chars[3] << "' unsupported by replace" << std::endl;
		return EXIT_FAILURE;
	} else {
		int (*processFunction)(std::istream&, const fs::path&);
		switch (magic.value()) {
			case 'PIC4': processFunction = processPic; break;
			case 'BUP4': processFunction = processBup; break;
			case 'TXA4': processFunction = processTxa; break;
			case 'MSK3': processFunction = processMsk3; break;
			case 'MSK4': processFunction = processMsk4; break;
			default: {
				char *chars = (char *)&magic;
				std::cerr << argv[1] << ": unknown magic: '" << chars[0] << chars[1] << chars[2] << chars[3] << "'" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		return processFunction(in, argv[2]);
	}
}


