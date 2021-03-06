#include <stdint.h>

#include <iostream>
#include <fstream>

#include <boost/endian/buffers.hpp>

#include "Config.hpp"
#include "FileTypes.hpp"

int usage(int argc, char **argv) {
	std::cerr << "Usage: " << argv[0] << " file.(pic|bup|txa|msk) file.png OPTIONS" << std::endl;
	std::cerr << "    Converts Switch and PS3 Higurashi picture file file.pic to PNG file.png" << std::endl;
	std::cerr << "Options:" << std::endl;
	std::cerr << "    -debug-images debugImagesFolder: Write individual chunks to the given folder for debugging" << std::endl;
	std::cerr << "    -bup-parts: Output separately combinable parts instead of precombined images when decoding bup files" << std::endl;
	exit(1);
}

const char* currentFileName = nullptr;
bool SHOULD_WRITE_DEBUG_IMAGES = false;
bool SAVE_BUP_AS_PARTS = false;
fs::path debugImagePath;

int main(int argc,char **argv){
	char *inFilename = NULL, *outFilename = NULL;
	for (int i = 1; i < argc; i++) {
		if (0 == strcmp(argv[i], "-bup-parts")) {
			SAVE_BUP_AS_PARTS = true;
		}
		else if (0 == strcmp(argv[i], "-debug-images")) {
			i++;
			if (i >= argc) usage(argc, argv);
			debugImagePath = argv[i];
			SHOULD_WRITE_DEBUG_IMAGES = true;
		}
		else if (!inFilename) {
			inFilename = argv[i];
		}
		else if (!outFilename) {
			outFilename = argv[i];
		}
		else {
			usage(argc, argv);
		}
	}
	if(!inFilename || !outFilename) usage(argc,argv);
	currentFileName = inFilename;

	std::ifstream in(argv[1]);
	if (!in) {
		std::cerr << "Failed to open file " << argv[1] << std::endl;
		exit(EXIT_FAILURE);
	}

	boost::endian::big_int32_buf_t magic;
	
	in.read((char *)&magic, 4);
	in.seekg(0, in.beg);

	int (*processFunction)(std::istream&, const fs::path&);

	switch(magic.value()){
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


