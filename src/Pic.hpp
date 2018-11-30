#pragma once

#include <stdint.h>
#include <vector>
#include <fstream>
#include <boost/filesystem.hpp>

#include "Config.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"

#if SHOULD_TEMPLATE
template <typename PicHeader>
#else
inline
#endif
int processPic(std::ifstream &in, const boost::filesystem::path &output) {
	PicHeader header;
	in.read((char *)&header, sizeof(header));

	std::vector<typename PicHeader::Chunk> chunks(header.chunks);
	in.read((char *)chunks.data(), chunks.size() * sizeof(chunks[0]));

	Image result({header.width, header.height}), currentChunk({0, 0});

	for(size_t i = 0; i < chunks.size(); i++) {
		const auto &chunk = chunks[i];
		processChunk(currentChunk, chunk.offset, in, "chunk" + std::to_string(i));
		currentChunk.drawOnto(result, {chunk.x, chunk.y}, currentChunk.size);
	}

	result.writePNG(output);
	return 0;
}
