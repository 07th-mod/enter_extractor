#include "HeaderStructs.hpp"

#include <array>
#include <boost/locale.hpp>
#include <boost/endian/buffers.hpp>
#include "Utilities.hpp"

typedef boost::endian::little_int32_buf_t int32_le;
typedef boost::endian::little_int16_buf_t int16_le;
typedef boost::endian::little_uint32_buf_t uint32_le;
typedef boost::endian::little_uint16_buf_t uint16_le;

static const std::locale cp932 = boost::locale::generator().generate("ja_JP.cp932");

template <typename T>
bool allZero(const T& t) {
	return std::all_of(t.begin(), t.end(), [](uint32_le x){ return x.value() == 0; });
};

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

struct MaskRectRaw {
	uint16_le x1;
	uint16_le y1;
	uint16_le x2;
	uint16_le y2;
	MaskRectRaw() = default;
	MaskRectRaw(MaskRect other): x1(other.x1), y1(other.y1), x2(other.x2), y2(other.y2) {}
	operator MaskRect() const {
		return { x1.value(), y1.value(), x2.value(), y2.value() };
	}
};
static_assert(sizeof(MaskRectRaw) == 8, "Expected MaskRectRaw to be 8 bytes");

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
	PicChunkSwitch() = default;
	PicChunkSwitch(uint16_t x, uint16_t y, uint32_t offset, uint32_t size): x(x), y(y), offset(offset), size(size) {}
	uint32_t getSize() { return size.value(); }
};
static_assert(sizeof(PicChunkSwitch) == 12, "Expected PicChunkSwitch to be 12 bytes");

struct PicChunkPS3 {
	uint16_le x;
	uint16_le y;
	uint32_le offset;
	PicChunkPS3() = default;
	PicChunkPS3(uint16_t x, uint16_t y, uint32_t offset, uint32_t size): x(x), y(y), offset(offset) {}
	uint32_t getSize() { return 0; }
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
	int getVersion() { return version.value(); }
	void setVersion(int v) { version = v; }
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
	int getVersion() { return 0; }
	void setVersion(int v) {}
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
	inline bool unkBytesValid() const { return allZero(unk); }
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

struct BupChunkV4 {
	uint32_le offset;
	uint32_le size;
	uint32_le unk;
};
static_assert(sizeof(BupChunkV4) == 12, "Expected BupChunkV4 to be 12 bytes");

struct BupExpressionChunkV4 {
	uint32_le headerLen;
	std::array<uint32_le, 3> unk;
	BupChunkV4 face;
	uint32_le numMouths;
	// name
	// BupChunkV4 mouths[numMouths]
	inline bool unkBytesValid() const { return allZero(unk); }
};
static_assert(sizeof(BupExpressionChunkV4) == 32, "Expected BupExpressionChunkV4 to be 32 bytes");

struct BupHeaderV4 {
	uint32_le magic;
	uint32_le version;
	uint32_le size;
	uint16_le ew;
	uint16_le eh;
	uint16_le width;
	uint16_le height;
	std::array<uint32_le, 11> unk;
};

// MARK: TXA

struct TxaChunkPS3 {
	uint16_le headerLength;
	uint16_le index;
	uint16_le width;
	uint16_le height;
	uint32_le entryOffset;
	uint32_le entryLength;
	uint32_t getDecLength() { return 0; }
	void setDecLength(uint32_t val) {}
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
	uint32_le decodedLength;
	uint32_t getDecLength() { return decodedLength.value(); }
	void setDecLength(uint32_t val) { decodedLength = val; }
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
	uint32_le largestDecodedChunk;
	uint32_le unk1;
	uint32_le unk2;
	uint32_le unk3;
	uint32_t getIndexSize() { return 0; }
	void setIndexSize(uint32_t val) {}
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
	uint32_le largestDecodedChunk;
	uint32_le indexSize;
	uint32_le unk2;
	uint32_t getIndexSize() { return indexSize.value(); }
	void setIndexSize(uint32_t val) { indexSize = val; }
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

// MARK: - Stream Readers/Writers

// readRaw will stop at a RAW_END if it's defined on a type
template <typename Thing, typename = decltype(Thing::RAW_END)>
Thing readRaw(std::istream& stream) {
	Thing t;
	stream.read((char*)&t, offsetof(Thing, RAW_END));
	return t;
}

template <typename Thing>
Thing readRaw(std::istream& stream) {
	Thing t;
	stream.read((char*)&t, sizeof(t));
	return t;
}

template <typename Thing, typename = decltype(Thing::RAW_END)>
void writeRaw(std::ostream& stream, const Thing& thing) {
	stream.write((char*)&thing, offsetof(Thing, RAW_END));
}

template <typename Thing>
void writeRaw(std::ostream& stream, const Thing& thing) {
	stream.write((char*)&thing, sizeof(Thing));
}

void addZeroes(std::ostream& stream, int len) {
	constexpr int BUFLEN = 64;
	char zero[BUFLEN] = {0};
	for (int i = 0; i < len; i += BUFLEN) {
		stream.write(zero, std::min(len - i, BUFLEN));
	}
}

void copyBytes(std::ostream& dst, std::istream& src, int len) {
	constexpr int BUFLEN = 512;
	char buf[BUFLEN];
	for (int i = 0; i < len; i += BUFLEN) {
		int amt = std::min(len - i, BUFLEN);
		src.read(buf, amt);
		dst.write(buf, amt);
	}
}

std::istream& operator>>(std::istream& stream, ChunkHeader& header) {
	auto h = readRaw<ChunkHeaderRaw>(stream);
	header.type = static_cast<ChunkHeader::Type>(h.type.value());
	header.masks.resize(h.masks.value());
	header.transparentMasks.resize(h.transparentMasks.value());
	header.alignmentWords = h.alignmentWords.value();
	header.x = h.x.value();
	header.y = h.y.value();
	header.w = h.w.value();
	header.h = h.h.value();
	header.size = h.size.value();
	for (auto& mask : header.masks) {
		mask = readRaw<MaskRectRaw>(stream);
	}
	for (auto& tmask : header.transparentMasks) {
		tmask = readRaw<MaskRectRaw>(stream);
	}
	stream.ignore(header.alignmentWords * 2);
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const ChunkHeader& header) {
	ChunkHeaderRaw h;
	h.type = static_cast<uint16_t>(header.type);
	h.masks = header.masks.size();
	h.transparentMasks = header.transparentMasks.size();
	h.alignmentWords = header.alignmentWords;
	h.x = header.x;
	h.y = header.y;
	h.w = header.w;
	h.h = header.h;
	h.size = header.size;
	writeRaw(stream, h);
	for (const auto& mask : header.masks) {
		MaskRectRaw m = mask;
		writeRaw(stream, m);
	}
	for (const auto& tmask : header.transparentMasks) {
		MaskRectRaw m = tmask;
		writeRaw(stream, m);
	}
	stream.seekp(header.alignmentWords * 2, stream.cur);
	return stream;
}

size_t ChunkHeader::calcAlignmentGetBinSize() {
	size_t binsize = sizeof(ChunkHeaderRaw) + sizeof(MaskRectRaw) * masks.size() + transparentMasks.size();
	size_t aligned = (binsize + 15) / 16 * 16;
	alignmentWords = (aligned - binsize) / 2;
	return aligned;
}

// MARK: PIC

template <typename Header>
void readPic(std::istream& stream, PicHeader& header) {
	header.isSwitch = Header::IsSwitch::value;
	Header h = readRaw<Header>(stream);
	header.filesize = h.filesize.value();
	header.version = h.getVersion();
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
		chunk.size = c.getSize();
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

template <typename Header>
size_t picBinSize(const PicHeader* pic) {
	Header h;
	h.setVersion(pic->version);
	return sizeof(Header) + h.bytesToSkip() + pic->chunks.size() * sizeof(typename Header::Chunk);
}

size_t PicHeader::binSize() const {
	if (isSwitch) {
		return picBinSize<PicHeaderSwitch>(this);
	} else {
		return picBinSize<PicHeaderPS3>(this);
	}
}

template <typename Header>
void writePic(std::ostream& stream, const PicHeader& header, std::istream& reference) {
	reference.seekg(0, reference.beg);
	Header h = readRaw<Header>(reference);
	h.filesize = header.filesize;
	h.ew = header.ew;
	h.eh = header.eh;
	h.width = header.width;
	h.height = header.height;
	h.chunks = header.chunks.size();
	writeRaw(stream, h);
	copyBytes(stream, reference, h.bytesToSkip());
	for (const auto& chunk : header.chunks) {
		typename Header::Chunk c(chunk.x, chunk.y, chunk.offset, chunk.size);
		writeRaw(stream, c);
	}
}

void PicHeader::write(std::ostream& stream, std::istream& reference) const {
	if (isSwitch) {
		writePic<PicHeaderSwitch>(stream, *this, reference);
	} else {
		writePic<PicHeaderPS3>(stream, *this, reference);
	}
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
	} else if (version < 4) { // Switch Old
		readBup<BupHeaderSwitch>(stream, header);
	} else { // Switch New
		header.isSwitch = true;
		auto h = readRaw<BupHeaderV4>(stream);
		header.eh = h.eh.value();
		header.ew = h.ew.value();
		header.width = h.width.value();
		header.height = h.height.value();
		auto numChunks = readRaw<uint32_le>(stream);
		header.chunks.resize(numChunks.value());

		for (auto& chunk : header.chunks) {
			auto c = readRaw<BupChunkV4>(stream);
			copyTo(chunk, c);
		}

		while (stream.tellg() % 16 != 0) {
			auto t = readRaw<uint32_le>(stream);
			if (t.value() != 0) {
				std::cerr << "Unexpected nonzero values in header" << std::endl;
			}
		}

		auto numExpChunks = readRaw<uint32_le>(stream);
		header.expChunks.resize(numExpChunks.value());
		std::vector<char> nameBuf;

		for (size_t i = 0; i < header.expChunks.size(); i++) {
			auto& expChunk = header.expChunks[i];
			auto c = readRaw<BupExpressionChunkV4>(stream);
			copyTo(expChunk.face, c.face);
			auto namelen = c.headerLen.value() - sizeof(c) - c.numMouths.value() * sizeof(BupChunkV4);
			nameBuf.resize(namelen + 1);
			nameBuf[namelen] = 0;
			stream.read(nameBuf.data(), namelen);
			expChunk.name = boost::locale::conv::to_utf<char>(nameBuf.data(), cp932);

			expChunk.mouths.resize(c.numMouths.value());
			for (auto& mouth : expChunk.mouths) {
				auto m = readRaw<BupChunkV4>(stream);
				copyTo(mouth, m);
			}
		}
	}
	return stream;
}

// MARK: TXA

template<typename Header>
void readTxa(std::istream& stream, TxaHeader& header) {
	header.isSwitch = Header::IsSwitch::value;
	Header h = readRaw<Header>(stream);
	header.indexed = h.indexed.value();
	header.largestDecodedChunk = h.largestDecodedChunk.value();
	header.indexSize = h.getIndexSize();
	header.chunks.resize(h.chunks.value());

	std::vector<char> buffer;

	for (auto& chunk : header.chunks) {
		auto c = readRaw<typename Header::Chunk>(stream);
		chunk.index = c.index.value();
		chunk.width = c.width.value();
		chunk.height = c.height.value();
		chunk.offset = c.entryOffset.value();
		chunk.length = c.entryLength.value();
		chunk.decodedLength = c.getDecLength();
		const size_t stringLength = c.headerLength.value() - sizeof(c);
		buffer.resize(stringLength + 1);
		buffer[stringLength] = 0;
		stream.read(buffer.data(), stringLength);
		chunk.name = boost::locale::conv::to_utf<char>(buffer.data(), cp932);
	}
}

size_t TxaHeader::updateAndCalcBinSize() {
	size_t size = isSwitch ? sizeof(TxaHeaderSwitch) : sizeof(TxaHeaderPS3);
	size_t chunksize = isSwitch ? sizeof(TxaChunkSwitch) : sizeof(TxaChunkPS3);
	indexSize = 0;
	largestDecodedChunk = 0;

	for (const auto& chunk : chunks) {
		indexSize += chunksize;
		indexSize += align(boost::locale::conv::from_utf(chunk.name, cp932).size() + 1, 4);
		if (chunk.decodedLength > largestDecodedChunk) {
			largestDecodedChunk = chunk.decodedLength;
		}
	}

	return size + indexSize;
}

template <typename Header>
void writeTxa(std::ostream& stream, const TxaHeader& header, std::istream& reference) {
	reference.seekg(0, reference.beg);
	Header h = readRaw<Header>(reference);
	h.size = header.filesize;
	h.setIndexSize(header.indexSize);
	h.indexed = header.indexed;
	h.largestDecodedChunk = header.largestDecodedChunk;

	writeRaw(stream, h);
	for (const auto& chunk : header.chunks) {
		typename Header::Chunk c;
		c.index = chunk.index;
		c.width = chunk.width;
		c.height = chunk.height;
		c.entryOffset = chunk.offset;
		c.entryLength = chunk.length;
		c.setDecLength(chunk.decodedLength);
		std::string s = boost::locale::conv::from_utf(chunk.name, cp932);
		s.resize(align(s.size() + 1, 4));
		c.headerLength = sizeof(c) + s.size();
		writeRaw(stream, c);
		stream.write(s.data(), s.size());
	}
}

void TxaHeader::write(std::ostream& stream, std::istream& reference) const {
	if (isSwitch) {
		writeTxa<TxaHeaderSwitch>(stream, *this, reference);
	} else {
		writeTxa<TxaHeaderPS3>(stream, *this, reference);
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
