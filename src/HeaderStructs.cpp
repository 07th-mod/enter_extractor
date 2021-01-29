#include "HeaderStructs.hpp"

#include <array>
#include <boost/locale.hpp>
#include <boost/endian/buffers.hpp>

typedef boost::endian::little_int32_buf_t int32_le;
typedef boost::endian::little_int16_buf_t int16_le;
typedef boost::endian::little_uint32_buf_t uint32_le;
typedef boost::endian::little_uint16_buf_t uint16_le;

static const std::locale cp932 = boost::locale::generator().generate("ja_JP.cp932");

/// Returns -1 if the file is PS3, the version number if the file is switch
static int detectSwitch(std::istream &in) {
	in.seekg(0, in.end);
	int size = in.tellg();
	in.seekg(4, in.beg);
	boost::endian::little_int32_buf_t buf[2];
	in.read((char *)buf, sizeof(buf));
	in.seekg(0, in.beg);
	if (buf[1].value() == size) {
		return buf[0].value();
	}
	else {
		if (buf[0].value() != size) {
			std::cerr << "Failed to autodetect file type of " << currentFileName << ", guessing PS3" << std::endl;
		}
		return -1;
	}
}

// MARK: - In-File Header Structs
#pragma pack(push, 1)

struct ChunkHeaderRaw {
	uint16_le type;
	uint16_le masks;
	uint16_le transparentMasks; // Masks that go over partially transparent sections of the image
	uint16_le alignmentWords;
	uint16_le x;
	uint16_le y;
	uint16_le w;
	uint16_le h;
	uint32_le size;
};
static_assert(sizeof(ChunkHeaderRaw) == 20, "Expected ChunkHeaderRaw to be 20 bytes");

// MARK: PIC

struct PicChunkSwitch {
	uint16_le x;
	uint16_le y;
	uint32_le offset;
	uint32_le size;
};
static_assert(sizeof(PicChunkSwitch) == 12, "Expected PicChunkSwitch to be 12 bytes");

struct PicChunkPS3 {
	uint16_le x;
	uint16_le y;
	uint32_le offset;
};
static_assert(sizeof(PicChunkPS3) == 8, "Expected PicChunkPS3 to be 8 bytes");

struct PicHeaderSwitch {
	typedef PicChunkSwitch Chunk;
	typedef std::true_type IsSwitch;
	uint32_le magic;
	uint32_le version;
	uint32_le filesize;
	uint16_le ew;
	uint16_le eh;
	uint16_le width;
	uint16_le height;
	uint32_le unk1;
	uint32_le chunks;
	uint32_le unk2;
	int bytesToSkip() { return version.value() >= 3 ? 4 : 0; }
};
static_assert(sizeof(PicHeaderSwitch) == 32, "Expected PicHeaderSwitch to be 32 bytes");

struct PicHeaderPS3 {
	typedef PicChunkPS3 Chunk;
	typedef std::false_type IsSwitch;
	uint32_le magic;
	uint32_le filesize;
	uint16_le ew;
	uint16_le eh;
	uint16_le width;
	uint16_le height;
	uint32_le unk1;
	uint32_le chunks;
	int bytesToSkip() { return 0; }
};
static_assert(sizeof(PicHeaderPS3) == 24, "Expected PicHeaderPS3 to be 24 bytes");

// MARK: BUP

struct BupChunkSwitch {
	uint32_le offset;
	uint32_le size;
	inline uint32_t getSize() const { return size.value(); }
};
static_assert(sizeof(BupChunkSwitch) == 8, "Expected BupChunkSwitch to be 8 bytes");

struct BupChunkPS3 {
	uint32_le offset;
	inline uint32_t getSize() const { return 0; }
};
static_assert(sizeof(BupChunkPS3) == 4, "Expected BupChunkPS3 to be 4 bytes");

struct BupExpressionChunkSwitch {
	char name[20];
	BupChunkSwitch face;
	std::array<uint32_le, 6> unk;
	std::array<BupChunkSwitch, 3> mouths;
	inline bool unkBytesValid() const { return std::all_of(unk.begin(), unk.end(), [](uint32_le x){ return x.value() == 0; }); }
};
static_assert(sizeof(BupExpressionChunkSwitch) == 76, "Expected BupExpressionChunkSwitch to be 76 bytes");

struct BupExpressionChunkPS3 {
	char name[16];
	BupChunkPS3 face;
	std::array<uint32_le, 3> unk;
	std::array<BupChunkPS3, 3> mouths;
	inline bool unkBytesValid() const { return true; }
};
static_assert(sizeof(BupExpressionChunkPS3) == 44, "Expected BupExpressionChunkPS3 to be 44 bytes");

struct BupHeaderSwitch {
	typedef BupChunkSwitch Chunk;
	typedef BupExpressionChunkSwitch ExpressionChunk;
	typedef std::true_type IsSwitch;
	static const int skipAmount = 12;
	static const int skipAmount2 = 12;
	uint32_le magic;
	uint32_le version;
	uint32_le size;
	uint16_le ew;
	uint16_le eh;
	uint16_le width;
	uint16_le height;
	uint32_le tbl1;
	uint32_le baseChunks;
	uint32_le expChunks;
	uint32_le unk1;
};
static_assert(sizeof(BupHeaderSwitch) == 36, "Expected BupHeaderSwitch to be 36 bytes");

struct BupHeaderPS3 {
	typedef BupChunkPS3 Chunk;
	typedef BupExpressionChunkPS3 ExpressionChunk;
	typedef std::false_type IsSwitch;
	static const int skipAmount = 4;
	static const int skipAmount2 = 0;
	uint32_le magic;
	uint32_le size;
	uint16_le ew;
	uint16_le eh;
	uint16_le width;
	uint16_le height;
	uint32_le tbl1;
	uint32_le baseChunks;
	uint32_le expChunks;
};
static_assert(sizeof(BupHeaderPS3) == 28, "Expected BupHeaderPS3 to be 28 bytes");

// MARK: TXA

struct TxaChunkPS3 {
	uint16_le headerLength;
	uint16_le index;
	uint16_le width;
	uint16_le height;
	uint32_le entryOffset;
	uint32_le entryLength;
	// name
};
static_assert(sizeof(TxaChunkPS3) == 16, "Expected TxaChunkPS3 to be 16 bytes");

struct TxaChunkSwitch {
	uint16_le headerLength;
	uint16_le index;
	uint16_le width;
	uint16_le height;
	uint32_le entryOffset;
	uint32_le entryLength;
	uint32_le unk;
	// name
};
static_assert(sizeof(TxaChunkSwitch) == 20, "Expected TxaChunkSwitch to be 20 bytes");

struct TxaHeaderPS3 {
	typedef TxaChunkPS3 Chunk;
	typedef std::false_type IsSwitch;
	uint32_le magic;
	uint32_le size;
	uint32_le indexed;
	uint32_le chunks;
	uint32_le decSize;
	uint32_le unk1;
	uint32_le unk2;
	uint32_le unk3;
};
static_assert(sizeof(TxaHeaderPS3) == 32, "Expected TxaChunkPS3 to be 32 bytes");

struct TxaHeaderSwitch {
	typedef TxaChunkSwitch Chunk;
	typedef std::true_type IsSwitch;
	uint32_le magic;
	uint32_le version;
	uint32_le size;
	uint32_le indexed;
	uint32_le chunks;
	uint32_le decSize;
	uint32_le unk1;
	uint32_le unk2;
};
static_assert(sizeof(TxaHeaderSwitch) == 32, "Expected TxaChunkSwitch to be 32 bytes");

// MARK: MSK

struct Msk3HeaderPS3 {
	typedef std::false_type IsSwitch;
	uint32_le magic;
	uint32_le size;
	uint16_le width;
	uint16_le height;
	uint32_le compressedSize;
};
static_assert(sizeof(Msk3HeaderPS3) == 16, "Expected Msk3HeaderPS3 to be 16 bytes");

struct Msk3HeaderSwitch {
	typedef std::true_type IsSwitch;
	uint32_le magic;
	uint32_le version;
	uint32_le size;
	uint16_le width;
	uint16_le height;
	uint32_le compressedSize;
	uint32_le unk[3];
};
static_assert(sizeof(Msk3HeaderSwitch) == 32, "Expected Msk3HeaderSwitch to be 32 bytes");

// MSK4 is only known to be used on Switch
struct Msk4HeaderSwitch {
	uint32_le magic;
	uint32_le version;
	uint32_le size;
	uint32_le unk0;
	uint16_le width;
	uint16_le height;
	uint32_le dataOffset;
	uint32_le dataSize;
};
static_assert(sizeof(Msk4HeaderSwitch) == 28, "Expected Msk4HeaderSwitch to be 28 bytes");

#pragma pack(pop)

// MARK: - IStream Readers

// readRaw will stop at a READ_RAW_END if it's defined on a type
template <typename Thing, typename = decltype(Thing::READ_RAW_END)>
Thing readRaw(std::istream& stream) {
	Thing t;
	stream.read((char*)&t, offsetof(Thing, READ_RAW_END));
	return t;
}

template <typename Thing>
Thing readRaw(std::istream& stream) {
	Thing t;
	stream.read((char*)&t, sizeof(t));
	return t;
}

std::istream& operator>>(std::istream& stream, ChunkHeader& header) {
	auto h = readRaw<ChunkHeaderRaw>(stream);
	header.type = h.type.value();
	header.masks = h.masks.value();
	header.transparentMasks = h.transparentMasks.value();
	header.alignmentWords = h.alignmentWords.value();
	header.x = h.x.value();
	header.y = h.y.value();
	header.w = h.w.value();
	header.h = h.h.value();
	header.size = h.size.value();
	return stream;
}

// MARK: PIC

template <typename Header>
void readPic(std::istream& stream, PicHeader& header) {
	header.isSwitch = Header::IsSwitch::value;
	Header h = readRaw<Header>(stream);
	header.eh = h.eh.value();
	header.ew = h.ew.value();
	header.width = h.width.value();
	header.height = h.height.value();
	header.chunks.resize(h.chunks.value());

	stream.ignore(h.bytesToSkip());

	for (auto& chunk : header.chunks) {
		auto c = readRaw<typename Header::Chunk>(stream);
		chunk.x = c.x.value();
		chunk.y = c.y.value();
		chunk.offset = c.offset.value();
	}
}

std::istream& operator>>(std::istream& stream, PicHeader& header) {
	int version = detectSwitch(stream);
	if (version < 0) { // PS3
		readPic<PicHeaderPS3>(stream, header);
	} else { // Switch
		readPic<PicHeaderSwitch>(stream, header);
	}
	return stream;
}

// MARK: BUP

template <typename Chunk>
void copyTo(BupChunk& out, const Chunk& in) {
	out.offset = in.offset.value();
};

// NOTE: Name must be copied separately
template <typename Chunk>
void copyTo(BupExpressionChunk& out, const Chunk& in) {
	copyTo(out.face, in.face);
	out.mouths.resize(in.mouths.size());
	for (size_t i = 0; i < in.mouths.size(); i++) {
		copyTo(out.mouths[i], in.mouths[i]);
	}
};

template <typename Header>
void copyTo(BupHeader& out, const Header& in) {
	out.eh = in.eh.value();
	out.ew = in.ew.value();
	out.width = in.width.value();
	out.height = in.height.value();
	out.chunks.resize(in.baseChunks.value());
	out.expChunks.resize(in.expChunks.value());
};

template <typename Header>
void readBup(std::istream& stream, BupHeader& header) {
	header.isSwitch = Header::IsSwitch::value;
	Header h = readRaw<Header>(stream);
	copyTo(header, h);

	// Skip bytes for unknown reason
	stream.ignore(h.tbl1.value() * h.skipAmount);

	for (auto& chunk : header.chunks) {
		auto c = readRaw<typename Header::Chunk>(stream);
		copyTo(chunk, c);
	}

	stream.ignore(h.skipAmount2);

	for (size_t i = 0; i < header.expChunks.size(); i++) {
		auto& expChunk = header.expChunks[i];

		auto c = readRaw<typename Header::ExpressionChunk>(stream);
		copyTo(expChunk, c);
		expChunk.name = boost::locale::conv::to_utf<char>(c.name, cp932);

		if (!c.unkBytesValid()) {
			std::cerr << "Expression " << i << ", " << expChunk.name << ", had unexpected nonzero values in its expression data..." << std::endl;
		}

		if (expChunk.name.empty()) {
			expChunk.name = "expression" + std::to_string(i);
		}
	}
}

std::istream& operator>> (std::istream& stream, BupHeader &header) {
	int version = detectSwitch(stream);
	if (version < 0) { // PS3
		readBup<BupHeaderPS3>(stream, header);
	} else { // Switch
		readBup<BupHeaderSwitch>(stream, header);
	}
	return stream;
}

// MARK: TXA

template<typename Header>
void readTxa(std::istream& stream, TxaHeader& header) {
	header.isSwitch = Header::IsSwitch::value;
	Header h = readRaw<Header>(stream);
	header.indexed = h.indexed.value();
	header.decSize = h.decSize.value();
	header.chunks.resize(h.chunks.value());

	for (auto& chunk : header.chunks) {
		auto c = readRaw<typename Header::Chunk>(stream);
		chunk.index = c.index.value();
		chunk.width = c.width.value();
		chunk.height = c.height.value();
		chunk.offset = c.entryOffset.value();
		chunk.length = c.entryLength.value();
		const size_t stringLength = c.headerLength.value() - sizeof(c);
		char buffer[stringLength + 1];
		buffer[stringLength] = 0;
		stream.read(buffer, stringLength);
		chunk.name = boost::locale::conv::to_utf<char>(buffer, cp932);
	}
}

std::istream& operator>>(std::istream& stream, TxaHeader& header) {
	int version = detectSwitch(stream);
	if (version < 0) { // PS3
		readTxa<TxaHeaderPS3>(stream, header);
	} else { // Switch
		readTxa<TxaHeaderSwitch>(stream, header);
	}
	return stream;
}

// MARK: MSK

template<typename Header>
void readMsk3(std::istream& stream, Msk3Header& header) {
	header.isSwitch = Header::IsSwitch::value;
	Header h = readRaw<Header>(stream);
	header.width = h.width.value();
	header.height = h.height.value();
	header.compressedSize = h.compressedSize.value();
}

std::istream& operator>>(std::istream& stream, Msk3Header& header) {
	int version = detectSwitch(stream);
	if (version < 0) { // PS3
		readMsk3<Msk3HeaderPS3>(stream, header);
	} else { // Switch
		readMsk3<Msk3HeaderSwitch>(stream, header);
	}
	return stream;
}

std::istream& operator>>(std::istream& stream, Msk4Header& header) {
	int version = detectSwitch(stream);
	if (version < 0) { // PS3
		throw std::runtime_error("PS3 MSK4 decoding is unsupported");
	} else { // Switch
		auto h = readRaw<Msk4HeaderSwitch>(stream);
		header.width = h.width.value();
		header.height = h.height.value();
		header.dataOffset = h.dataOffset.value();
		header.dataSize = h.dataSize.value();
	}
	return stream;
}
