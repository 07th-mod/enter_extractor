#include <stdint.h>

#include <iostream>
#include <fstream>
#include <vector>

#include <boost/endian/buffers.hpp>

#include "Config.hpp"
#include "Pic.hpp"
#include "Bup.hpp"
#include "Txa.hpp"

int usage(int argc, char **argv) {
	std::cerr << "Usage: " << argv[0] << " file.pic file.png" << std::endl;
	std::cerr << "    Converts Switch and PS3 Higurashi picture file file.pic to PNG file.png" << std::endl;
	exit(1);
}

int main(int argc,char **argv){
	if(argc!=3) usage(argc,argv);
	std::ifstream in(argv[1]);
	if (!in) {
		std::cerr << "Failed to open file " << argv[1] << std::endl;
		exit(EXIT_FAILURE);
	}

	boost::endian::big_int32_buf_t magic;
	
	in.read((char *)&magic, 4);
	in.seekg(0, in.beg);

	switch(magic.value()){
#if SHOULD_TEMPLATE
		case 'PIC3': return processPic<PicHeaderPS3>(in, argv[2]);
		case 'PIC4': return processPic<PicHeaderSwitch>(in, argv[2]);
		case 'BUP3': return processBup<BupHeaderPS3>(in, argv[2]);
		case 'BUP4': return processBup<BupHeaderSwitch>(in, argv[2]);
		case 'TXA3': return processTxa<TxaHeaderPS3>(in, argv[2]);
		case 'TXA4': return processTxa<TxaHeaderSwitch>(in, argv[2]);
#else
		case 'PIC4': return processPic(in, argv[2]);
		case 'BUP4': return processBup(in, argv[2]);
		case 'TXA4': return processTxa(in, argv[2]);
#endif
		default: {
			char *chars = (char *)&magic;
			std::cerr << argv[1] << ": unknown magic: '" << chars[0] << chars[1] << chars[2] << chars[3] << "'" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}


