#include "FileTypes.hpp"

#include <stdint.h>
#include <unordered_map>
#include <algorithm>

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

/// Attempt to shrink a chunk that has empty (zero alpha) borders
Point shrinkChunk(Image& chunk) {
	Point start {0, 0};
	Point end {chunk.size.width, chunk.size.height};
	for (; start.y < end.y; start.y++) { // Shrink from top
		if (std::any_of(&chunk.pixel(start.x, start.y), &chunk.pixel(end.x, start.y), [](Color c){ return c.a > 0; })) {
			break;
		}
	}
	for (; start.y < end.y - 1; end.y--) { // Shrink from bottom
		if (std::any_of(&chunk.pixel(start.x, end.y - 1), &chunk.pixel(end.x, end.y - 1), [](Color c){ return c.a > 0; })) {
			break;
		}
	}
	for (; start.x < end.x; start.x++) { // Shrink from left
		for (int y = start.y; y < end.y; y++) {
			if (chunk.pixel(start.x, y).a > 0)
				goto leftfound;
		}
	}
leftfound:
	for(; start.x < end.x - 1; end.x--) { // Shrink from right
		for (int y = start.y; y < end.y; y++) {
			if (chunk.pixel(end.x - 1, y).a > 0)
				goto rightfound;
		}
	}
rightfound:
	if (start == Point{0, 0} && end == Point{chunk.size.width, chunk.size.height}) {
		// No resizing possible
		return {0, 0};
	} else if (start.y == end.y) {
		// TODO: Be able to remove empty chunks
		chunk.fastResize({1, 1});
		return {0, 0};
	} else {
		Image out({end.x - start.x, end.y - start.y});
		chunk.drawOnto(out, {0, 0}, {start.x, start.y}, out.size);
		chunk = std::move(out);
		return {start.x, start.y};
	}
}

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
			Point shrink = shrinkChunk(chunk);

			auto& headerEntry = header.chunks[headerIdx];
			headerIdx++;
			headerEntry.x = x1 + shrink.x;
			headerEntry.y = y1 + shrink.y;

			auto found = seen.find(chunk);
			if (found == seen.end()) {
				pos = (pos + 15) / 16 * 16;
				MaskRect bounds = {0, 0, static_cast<uint16_t>(chunk.size.width), static_cast<uint16_t>(chunk.size.height)};
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
