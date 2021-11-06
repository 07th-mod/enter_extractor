#include "FileTypes.hpp"

#include <stdint.h>
#include <unordered_map>

#include "Config.hpp"
#include "FS.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"

int processPic(std::istream &in, const fs::path &output) {
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

struct image_hash {
	std::size_t operator()(const Image& img) const {
		const char* beg = reinterpret_cast<const char*>(&*img.colorData.begin());
		const char* end = reinterpret_cast<const char*>(&*img.colorData.end());
#if __cplusplus >= 201703L
		return std::hash<std::string_view>()(std::string_view(beg, end - beg));
#else
		return std::hash<std::string>()(std::string(beg, end));
#endif
	}
};

int replacePic(std::istream &in, std::ostream &output, const fs::path &replacementFile) {
	PicHeader header;
	in >> header;

	int CHUNK_WIDTH = 1024;
	int CHUNK_HEIGHT = 1024;

	for (const auto& chunk : header.chunks) {
		if (chunk.x > 64 && chunk.x < CHUNK_WIDTH) {
			CHUNK_WIDTH = chunk.x;
		}
		if (chunk.y > 64 && chunk.y < CHUNK_HEIGHT) {
			CHUNK_HEIGHT = chunk.y;
		}
	}

	printf("Using %dx%d chunks\n", CHUNK_WIDTH, CHUNK_HEIGHT);

	std::unordered_map<Image, std::pair<uint32_t, uint32_t>, image_hash> seen;

	Compressor compressor;
	Image replacement = Image::readPNG(replacementFile);
	struct ChunkData {
		ChunkHeader header;
		std::vector<uint8_t> data;
		uint32_t offset;
	};
	std::vector<ChunkData> data;
	std::vector<PicChunk> chunkEntries;
	Size sizeInChunks = {(replacement.size.width + CHUNK_WIDTH - 1) / CHUNK_WIDTH, (replacement.size.height + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT};
	header.chunks.resize(sizeInChunks.area());
	int pos = header.binSize();
	Image chunk;
	int headerIdx = 0;
	for (int y1 = 0; y1 < replacement.size.height; y1 += CHUNK_HEIGHT) {
		int height = std::min(replacement.size.height - y1, CHUNK_HEIGHT);
		for (int x1 = 0; x1 < replacement.size.width; x1 += CHUNK_WIDTH) {
			int width = std::min(replacement.size.width - x1, CHUNK_WIDTH);
			chunk.fastResize({width, height});
			replacement.drawOnto(chunk, {0, 0}, {x1, y1}, {width, height});

			auto& headerEntry = header.chunks[headerIdx];
			headerIdx++;
			headerEntry.x = x1;
			headerEntry.y = y1;

			auto found = seen.find(chunk);
			if (found == seen.end()) {
				pos = (pos + 15) / 16 * 16;
				MaskRect bounds = {0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height)};
				data.emplace_back();
				auto& chunkEntry = data.back();
				chunkEntry.header = compressor.encodeChunk(chunkEntry.data, chunk, bounds, {0, 0}, header.isSwitch);
				chunkEntry.offset = pos;

				headerEntry.offset = pos;
				headerEntry.size = chunkEntry.header.calcAlignmentGetBinSize() + chunkEntry.data.size();
				seen.insert(std::make_pair(std::move(chunk), std::make_pair(headerEntry.offset, headerEntry.size)));
				pos += headerEntry.size;
			} else {
				headerEntry.offset = found->second.first;
				headerEntry.size = found->second.second;
			}
		}
	}

	header.filesize = pos;
	header.write(output, in);
	for (auto& chunk : data) {
		output.seekp(chunk.offset, output.beg);
		output << chunk.header;
		output.write(reinterpret_cast<char*>(chunk.data.data()), chunk.data.size());
	}
	return 0;
}
