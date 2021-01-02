#include <stdint.h>

#include <iostream>
#include <fstream>
#include <vector>

#include <boost/endian/buffers.hpp>

#include "Config.hpp"
#include "Pic.hpp"
#include "Bup.hpp"
#include "Txa.hpp"
#include "Msk3.hpp"
#include "Msk4.hpp"

int usage(int argc, char **argv) {
	std::cerr << "Usage: " << argv[0] << " file.pic file.png [-debug-images debugImagesFolder]" << std::endl;
	std::cerr << "    Converts Switch and PS3 Higurashi picture file file.pic to PNG file.png" << std::endl;
	exit(1);
}

#if SHOULD_TEMPLATE
static bool detectSwitch(std::ifstream &in, char *inName) {
	in.seekg(0, in.end);
	int size = in.tellg();
	in.seekg(4, in.beg);
	boost::endian::little_int32_buf_t buf[2];
	in.read((char *)buf, sizeof(buf));
	if (buf[0].value() == size) {
		return false;
	}
	else {
		if (buf[1].value() != size) {
			std::cerr << "Failed to autodetect file type of " << inName << ", guessing Switch"<< std::endl;
		}
		return true;
	}
}
#endif

bool SHOULD_WRITE_DEBUG_IMAGES = false;
boost::filesystem::path debugImagePath;

int main(int argc,char **argv){
	char *inFilename = NULL, *outFilename = NULL;
	for (int i = 1; i < argc; i++) {
		if (0 == strcmp(argv[i], "-debug-images")) {
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

	std::ifstream in(argv[1]);
	if (!in) {
		std::cerr << "Failed to open file " << argv[1] << std::endl;
		exit(EXIT_FAILURE);
	}

	boost::endian::big_int32_buf_t magic;
	
	in.read((char *)&magic, 4);
#if SHOULD_TEMPLATE
	bool isSwitch = detectSwitch(in, argv[1]);
#endif
	in.seekg(0, in.beg);

	int (*processFunction)(std::ifstream&, const boost::filesystem::path&);

	switch(magic.value()){
#if SHOULD_TEMPLATE
		case 'PIC4': processFunction = isSwitch ? processPic<PicHeaderSwitch> : processPic<PicHeaderPS3>; break;
		case 'BUP4': processFunction = isSwitch ? processBup<BupHeaderSwitch> : processBup<BupHeaderPS3>; break;
		case 'TXA4': processFunction = isSwitch ? processTxa<TxaHeaderSwitch> : processTxa<TxaHeaderPS3>; break;
		case 'MSK3': processFunction = isSwitch ? processMsk3<Msk3HeaderSwitch> : processMsk3<Msk3HeaderPS3>; break;
#else
		case 'PIC4': processFunction = processPic; break;
		case 'BUP4': processFunction = processBup; break;
		case 'TXA4': processFunction = processTxa; break;
		case 'MSK3': processFunction = processMsk3; break;
#endif
		case 'MSK4': processFunction = processMsk4; break;
		default: {
			char *chars = (char *)&magic;
			std::cerr << argv[1] << ": unknown magic: '" << chars[0] << chars[1] << chars[2] << chars[3] << "'" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	return processFunction(in, argv[2]);
}


