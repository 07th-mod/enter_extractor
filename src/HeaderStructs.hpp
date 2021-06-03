#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <istream>
#include "Config.hpp"

struct MaskRect {
	uint16_t x1;
	uint16_t y1;
	uint16_t x2;
	uint16_t y2;
};

struct ChunkHeader {
	enum Type : uint16_t {
		TYPE_COLOR1        = 0, ///< Only observed in PS3, no noticeable difference between it and `TYPE_COLOR`
		TYPE_COLOR         = 1, ///< RGBA
		TYPE_INDEXED_ALPHA = 2, ///< Indexed with a separate alpha mask
		TYPE_INDEXED       = 3, ///< Indexed, only type observed to be used uncompressed
	};
	uint16_t type;
	std::vector<MaskRect> masks;
	std::vector<MaskRect> transparentMasks; // Masks that go over partially transparent sections of the image
	uint16_t alignmentWords;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	uint32_t size;
};

std::istream& operator>>(std::istream& stream, ChunkHeader& header);

struct PicChunk {
	uint16_t x;
	uint16_t y;
	uint32_t offset;
};

struct PicHeader {
	bool isSwitch;
	uint16_t ew;
	uint16_t eh;
	uint16_t width;
	uint16_t height;
	std::vector<PicChunk> chunks;
};

std::istream& operator>>(std::istream& stream, PicHeader& header);

struct BupChunk {
	uint32_t offset;
};

struct BupExpressionChunk {
	std::string name;
	BupChunk face;
	std::vector<BupChunk> mouths;
};

struct BupHeader {
	bool isSwitch;
	uint16_t ew;
	uint16_t eh;
	uint16_t width;
	uint16_t height;
	std::vector<BupChunk> chunks;
	std::vector<BupExpressionChunk> expChunks;
};

std::istream& operator>>(std::istream& stream, BupHeader& header);

struct TxaChunk {
	uint16_t index;
	uint16_t width;
	uint16_t height;
	uint32_t offset;
	uint32_t length;
	std::string name;
};

struct TxaHeader {
	bool isSwitch;
	uint32_t indexed;
	uint32_t decSize;
	std::vector<TxaChunk> chunks;
};

std::istream& operator>>(std::istream& stream, TxaHeader& header);

struct Msk3Header {
	bool isSwitch;
	uint16_t width;
	uint16_t height;
	uint32_t compressedSize;
};

std::istream& operator>>(std::istream& stream, Msk3Header& header);

struct Msk4Header {
	uint16_t width;
	uint16_t height;
	uint32_t dataOffset;
	uint32_t dataSize;
};

std::istream& operator>>(std::istream& stream, Msk4Header& header);
