#pragma once

#include <stdint.h>
#include <array>
#include "Config.hpp"

#pragma pack(push, 1)

struct PicChunkSwitch {
	uint16_t x;
	uint16_t y;
	uint32_t offset;
	uint32_t size;
};
static_assert(sizeof(PicChunkSwitch) == 12, "Expected PicChunkSwitch to be 12 bytes");

struct PicChunkPS3 {
	uint16_t x;
	uint16_t y;
	uint32_t offset;
};
static_assert(sizeof(PicChunkPS3) == 8, "Expected PicChunkPS3 to be 8 bytes");

struct ChunkHeader {
	uint16_t type;
	uint16_t masks;
	uint16_t transparentMasks; // Masks that go over partially transparent sections of the image
	uint16_t unk;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	uint32_t size;
};
static_assert(sizeof(ChunkHeader) == 20, "Expected PicChunkHeader to be 20 bytes");

struct PicHeaderSwitch {
	typedef PicChunkSwitch Chunk;
	uint32_t magic;
	uint32_t unk0;
	uint32_t filesize;
	uint16_t ew;
	uint16_t eh;
	uint16_t width;
	uint16_t height;
	uint32_t unk1;
	uint32_t chunks;
	uint32_t unk2;
};
static_assert(sizeof(PicHeaderSwitch) == 32, "Expected PicHeaderSwitch to be 32 bytes");

struct PicHeaderPS3 {
	typedef PicChunkPS3 Chunk;
	uint32_t magic;
	uint32_t filesize;
	uint16_t ew;
	uint16_t eh;
	uint16_t width;
	uint16_t height;
	uint32_t unk1;
	uint32_t chunks;
};
static_assert(sizeof(PicHeaderPS3) == 24, "Expected PicHeaderPS3 to be 24 bytes");

struct BupChunkSwitch {
	uint32_t offset;
	uint32_t size;
	inline uint32_t getSize() const { return size; }
};
static_assert(sizeof(BupChunkSwitch) == 8, "Expected BupChunkSwitch to be 8 bytes");

struct BupChunkPS3 {
	uint32_t offset;
	inline uint32_t getSize() const { return 0; }
};
static_assert(sizeof(BupChunkPS3) == 4, "Expected BupChunkPS3 to be 4 bytes");

struct BupExpressionChunkSwitch {
	char name[20];
	BupChunkSwitch face;
	std::array<uint32_t, 6> unk;
	std::array<BupChunkSwitch, 3> mouths;
	inline bool unkBytesValid() const { return std::all_of(unk.begin(), unk.end(), [](uint32_t x){ return x == 0; }); }
};
static_assert(sizeof(BupExpressionChunkSwitch) == 76, "Expected BupExpressionChunkSwitch to be 76 bytes");

struct BupExpressionChunkPS3 {
	char name[16];
	BupChunkPS3 face;
	std::array<uint32_t, 3> unk;
	std::array<BupChunkPS3, 3> mouths;
	inline bool unkBytesValid() const { return true; }
};
static_assert(sizeof(BupExpressionChunkPS3) == 44, "Expected BupExpressionChunkPS3 to be 44 bytes");

struct BupHeaderSwitch {
	typedef BupChunkSwitch Chunk;
	typedef BupExpressionChunkSwitch ExpressionChunk;
	static const int skipAmount = 12;
	static const int skipAmount2 = 12;
	uint32_t magic;
	uint32_t unk0;
	uint32_t size;
	uint16_t ew;
	uint16_t eh;
	uint16_t width;
	uint16_t height;
	uint32_t tbl1;
	uint32_t baseChunks;
	uint32_t expChunks;
	uint32_t unk1;
};
static_assert(sizeof(BupHeaderSwitch) == 36, "Expected BupHeaderSwitch to be 36 bytes");

struct BupHeaderPS3 {
	typedef BupChunkPS3 Chunk;
	typedef BupExpressionChunkPS3 ExpressionChunk;
	static const int skipAmount = 4;
	static const int skipAmount2 = 0;
	uint32_t magic;
	uint32_t size;
	uint16_t ew;
	uint16_t eh;
	uint16_t width;
	uint16_t height;
	uint32_t tbl1;
	uint32_t baseChunks;
	uint32_t expChunks;
};
static_assert(sizeof(BupHeaderPS3) == 28, "Expected BupHeaderPS3 to be 28 bytes");

#pragma pack(pop)

#if !SHOULD_TEMPLATE
typedef PicHeaderSwitch PicHeader;
typedef BupHeaderSwitch BupHeader;
#endif
