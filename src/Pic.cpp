#include "FileTypes.hpp"

#include <stdint.h>
#include <vector>
#include <fstream>
#include <boost/filesystem.hpp>

#include "Config.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"

int processPic(std::ifstream &in, const boost::filesystem::path &output) {
	PicHeader header;
	in >> header;

	Image result({header.width, header.height}), currentChunk({0, 0});
	std::vector<MaskRect> maskData;

	for(size_t i = 0; i < header.chunks.size(); i++) {
		const auto &chunk = header.chunks[i];
		processChunk(currentChunk, maskData, chunk.offset, in, "chunk" + std::to_string(i), header.isSwitch);
		currentChunk.drawOnto(result, {chunk.x, chunk.y}, maskData);
	}

	result.writePNG(output);
	return 0;
}
