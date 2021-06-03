#include "Decompression.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <unordered_map>
#include "HeaderStructs.hpp"

static Size align(Size size) {
	return { (size.width + 3) & ~3, size.height };
}

static void swapBR(Image &image) {
	for (auto& color : image.colorData) {
		std::swap(color.b, color.r);
	}
}

bool decompressHigu(std::vector<uint8_t> &output, const uint8_t *input, int inputLength, bool isSwitch) {
	int marker = 1;
	int p = 0;

	output.clear();

	while (p < inputLength) {
		if (marker == 1) {
			marker = 0x100 | input[p++];
		}

		if (p >= inputLength) {
			break;
		}

		if (marker & 1) {
			if (p + 2 > inputLength) {
				return false;
			}
			uint8_t b1 = input[p++];
			uint8_t b2 = input[p++];

			if (isSwitch) {
				b1 = (b1 >> 4) | (b1 << 4);
			}

			int count = (b1 & 0x0F) + 3;
			int offset = ((b1 & 0xF0) << 4) | b2;

			for (int i = 0; i < count; i++) {
				ptrdiff_t index = output.size() - (offset + 1);
				if (index < 0) {
					return false;
				}
				output.push_back(output[index]);
			}
		}
		else {
			output.push_back(input[p++]);
		}

		marker >>= 1;
	}
	return true;
}

void getRGB(Image &image, const std::vector<uint8_t> &data, bool isSwitch) {
	int scanline = sizeof(Color) * image.size.width;
	uint8_t *start = (uint8_t *)image.colorData.data();
	memcpy(start, data.data(), scanline);
	for (int i = scanline; i < sizeof(Color) * image.colorData.size(); i++) {
		start[i] = start[i - scanline] + data[i];
	}
	if (!isSwitch) { swapBR(image); }
}

void prepareWriteRGB(std::vector<uint8_t> &output, const Image &image) {
	output.resize(image.colorData.size() * sizeof(Color));
	int scanline = sizeof(Color) * image.size.width;
	uint8_t *start = (uint8_t *)image.colorData.data();
	memcpy(output.data(), start, scanline);
	for (int i = scanline; i < sizeof(Color) * image.colorData.size(); i++) {
		output[i] = start[i] - start[i - scanline];
	}
}

bool getIndexed(Image &output, const std::vector<uint8_t> &input, Size size, bool isSwitch, int colors) {
	assert(input.size() >= colors * 4 + size.area());
	int imgStart = colors * 4;
	int maskStart = imgStart + size.area();
	std::vector<Color> table((Color *)&input[0], (Color *)&input[imgStart]);

	output.size = size;
	output.colorData.clear();
	for (int i = imgStart; i < maskStart; i++) {
		output.colorData.push_back(table[input[i]]);
	}
	for (int i = maskStart; i < input.size(); i++) {
		int x = (i - maskStart) % size.width;
		int y = (i - maskStart) / size.width;
		output.pixel(x, y).a = input[i];
	}

	if (!isSwitch) { swapBR(output); }
	return maskStart != input.size();
}

static void printDebugAndWrite(const Image &currentOutput, const ChunkHeader &header, const std::vector<MaskRect> &maskData, const std::string &name, std::istream &file) {
	std::cout << "========== " << name << " ==========" << std::endl;
	std::cout << "               Type " << header.type << std::endl;
	std::cout << "              Masks " << header.masks.size() << std::endl;
	std::cout << "  Transparent Masks " << header.transparentMasks.size() << std::endl;
	std::cout << "    Alignment Words " << header.alignmentWords << std::endl;
	std::cout << "                X Y " << header.x << " " << header.y << std::endl;
	std::cout << "                W H " << header.w << " " << header.h << std::endl;
	std::cout << "               Size " << header.size << std::endl;

	currentOutput.writePNG(debugImagePath/(name + ".png"));
	Image masked(currentOutput.size);
	currentOutput.drawOnto(masked, {0, 0}, maskData);
	masked.writePNG(debugImagePath/(name + "_masked.png"));
}

static void processChunkShared(Image &output, uint32_t size, ChunkHeader::Type type, int width, int height, std::istream &file, const std::string &name, bool isSwitch) {
	std::vector<uint8_t> decompressed;
	Size alignedSize = align({width, height});

	// If size is zero, then we're uncompressed and paletted
	if (size == 0) {
		if (type != ChunkHeader::TYPE_INDEXED) {
			throw std::runtime_error("Expected a size-0 type to be 3 (indexed, no alpha) but it wasn't...");
		}
		size = 1024 + alignedSize.area();
		std::vector<uint8_t> chunk(size);
		file.read((char *)chunk.data(), chunk.size());
		getIndexed(output, chunk, alignedSize, isSwitch);
		return;
	}
	else {
		std::vector<uint8_t> compressed(size);
		file.read((char *)compressed.data(), compressed.size());
		if (!decompressHigu(decompressed, compressed.data(), (int)compressed.size(), isSwitch)) {
			throw std::runtime_error("Decompression of " + name + " failed");
		}
	}

	if (type == ChunkHeader::TYPE_INDEXED || type == ChunkHeader::TYPE_INDEXED_ALPHA) {
		getIndexed(output, decompressed, alignedSize, isSwitch);
	}
	else {
		output.fastResize(alignedSize);
		int byteSize = output.size.area() * sizeof(Color);
		if (decompressed.size() < byteSize) {
			fprintf(stderr, "Decompressed too little data for chunk, have %zd but need %d bytes!\n", decompressed.size(), byteSize);
			decompressed.resize(byteSize);
		}
		else if (decompressed.size() > byteSize) {
			fprintf(stderr, "Decompressed too much data for chunk, have %zd but only need %d bytes!\n", decompressed.size(), byteSize);
		}
		getRGB(output, decompressed, isSwitch);
	}
}

void processChunkNoHeader(Image &output, uint32_t offset, uint32_t size, int indexed, int width, int height, std::istream &file, const std::string &name, bool isSwitch) {
	file.seekg(offset);

	ChunkHeader::Type type = indexed ? ChunkHeader::TYPE_INDEXED : ChunkHeader::TYPE_COLOR;
	processChunkShared(output, size, type, width, height, file, name, isSwitch);
}

Point processChunk(Image &output, std::vector<MaskRect> &outputMasks, uint32_t offset, std::istream &file, const std::string &name, bool isSwitch) {
	file.seekg(offset, file.beg);
	ChunkHeader header;
	file >> header;

	outputMasks.reserve(header.masks.size() + header.transparentMasks.size());
	outputMasks = header.masks;
	outputMasks.insert(outputMasks.end(), header.transparentMasks.begin(), header.transparentMasks.end());

	processChunkShared(output, header.size, header.type, header.w, header.h, file, name, isSwitch);

	if (SHOULD_WRITE_DEBUG_IMAGES) {
		printDebugAndWrite(output, header, outputMasks, name, file);
	}

	return {header.x, header.y};
}

void debugDecompress(uint32_t offset, uint32_t size, std::istream &in, bool isSwitch) {
	std::vector<uint8_t> data(size);
	std::vector<uint8_t> output;
	in.seekg(offset, in.beg);
	ChunkHeader header;
	in >> header;
	in.seekg(offset, in.beg);
	in.read((char *)data.data(), data.size());

	for (int i = 0; i < data.size(); i++) {
		if (decompressHigu(output, data.data() + i, (int)data.size() - i, isSwitch)) {
			std::ofstream out("/tmp/chunks/data" + std::to_string(i) + ".dat", std::ios::binary);
			out.write((char *)output.data(), output.size());
			if (output.size() >= 1024 + header.w * header.h) {
				Image img({0, 0});
				getIndexed(img, output, align({header.w, header.h}), isSwitch);
				img.writePNG("/tmp/chunks/data" + std::to_string(i) + ".png");
			}
		}
	}
}

// MARK: Compression

class LZ77Compressor {
	constexpr static int HASH_SHIFT = 1;
	constexpr static int HASH_BITS = 8 + 2 * HASH_SHIFT;
	static int hash(const uint8_t* bytes) {
		int res = bytes[0];
		res ^= bytes[1] << HASH_SHIFT;
		res ^= bytes[2] << (HASH_SHIFT * 2);
		return res;
	}

	int maxMatch;
	int windowLength;

public:
	struct Item {
		bool isRepeat;
		union {
			uint8_t value;
			struct {
				uint8_t length;
				uint16_t offset;
			};
		};
	};

private:
	std::vector<Item> output;

	// This is probably the most computationally intensive LZ77 compressor ever but oh well
	std::vector<int> dict[1 << HASH_BITS];

	void clear() {
		output.clear();
		for (auto& v : dict) { v.clear(); }
	}

	void add(const uint8_t* bytes, int offset) {
		dict[hash(bytes + offset)].push_back(offset);
	}

	void remove(const uint8_t* bytes, int offset) {
		auto& v = dict[hash(bytes + offset)];
		for (size_t i = 0; i < v.size(); i++) {
			if (v[i] == offset) {
				v[i] = v.back();
				v.pop_back();
				return;
			}
		}
	}

	bool search(const uint8_t* bytes, int srcOffset, int maxLen, int& matchOff, int& matchLen) {
		auto& v = dict[hash(bytes + srcOffset)];

		matchOff = -1;
		matchLen = 2;

		for (int off : v) {
			int i = 0;
			if (off + maxLen > srcOffset) {
				// Unlikely, will wrap
				for (int off2 = off; i < maxLen; i++, off2++) {
					if (off2 >= srcOffset) {
						off2 = off;
					}
					if (bytes[off2] != bytes[srcOffset + i]) { break; }
				}
			} else {
				for (; i < maxLen; i++) {
					if (bytes[off + i] != bytes[srcOffset + i]) { break; }
				}
			}
			if (i > matchLen) {
				matchOff = off;
				matchLen = i;
				if (matchLen == maxLen) { break; }
			}
		}

		return matchLen > 2;
	}

	void next(const uint8_t* input, int& i, int maxLen, int end) {
		int matchOff, matchLen;
		Item item = {0};
		if (search(input, i, maxLen, matchOff, matchLen)) {
			item.isRepeat = true;
			item.length = matchLen - 3;
			item.offset = (i - matchOff) - 1;
			output.push_back(item);
			for (int j = 0; j < matchLen; j++) {
				if (i + j >= windowLength) {
					remove(input, i + j - windowLength);
				}
				if (i + j < end) {
					add(input, i + j);
				}
			}
			i += matchLen;
		} else {
			item.isRepeat = false;
			item.value = input[i];
			output.push_back(item);
			if (i >= windowLength) {
				remove(input, i - windowLength);
			}
			add(input, i);
			i++;
		}
	}

public:
	LZ77Compressor(int matchBits = 4, int windowBits = 12): maxMatch((1 << matchBits) + 2), windowLength(1 << windowBits) {}

	void configure(int matchBits, int windowBits) {
		maxMatch = (1 << matchBits) + 2;
		windowLength = 1 << windowBits;
	}

	const std::vector<Item>& compress(const uint8_t* input, int inputLength) {
		clear();
		int i = 0;
		int matchOff, matchLen;
		while (i < inputLength - maxMatch) {
			next(input, i, maxMatch, inputLength - 2);
		}
		while (i < inputLength - 2) {
			next(input, i, inputLength - i, inputLength - 2);
		}
		for (; i < inputLength; i++) {
			Item item = {0};
			item.isRepeat = false;
			item.value = input[i];
			output.push_back(item);
		}
		return output;
	}

private:
	template <typename Fn>
	void compress_helper(std::vector<uint8_t>& out, const Item* items, int len, Fn& encode) {
		uint8_t control = 0;
		for (int i = 0; i < len; i++) {
			control |= ((items[i].isRepeat ? 1 : 0) << i);
		}
		out.push_back(control);
		for (int i = 0; i < len; i++) {
			encode(out, items[i]);
		}
	}

public:
	template <typename Fn>
	void compress(std::vector<uint8_t>& out, const uint8_t* input, int inputLength, Fn&& encode) {
		out.clear();
		const auto& vec = compress(input, inputLength);
		int i = 0;
		for (; i < static_cast<int>(vec.size()) - 7; i += 8) {
			compress_helper(out, &vec[i], 8, encode);
		}
		if (i < vec.size()) {
			compress_helper(out, &vec[i], vec.size() - i, encode);
		}
	}

	/// Encoding used by PIC, BUP, TXA, etc
	struct DefaultEncode {
		bool isSwitch;

		void operator()(std::vector<uint8_t>& out, Item item) {
			if (item.isRepeat) {
				uint8_t b1 = item.length | ((item.offset >> 4) & 0xF0);
				uint8_t b2 = item.offset & 0xFF;
				if (isSwitch) {
					b1 = (b1 >> 4) | (b1 << 4);
				}
				out.push_back(b1);
				out.push_back(b2);
			} else {
				out.push_back(item.value);
			}
		}
	};
};

namespace std {
	template <>
	struct hash<Color> {
		size_t operator()(Color c) const {
			static_assert(sizeof(int32_t) == sizeof(Color), "Should both be 4 bytes");
			int32_t i;
			memcpy(&i, &c, sizeof(i));
			return hash<int32_t>()(i);
		}
	};
}

struct CompressorImpl {
	std::unordered_map<Color, uint8_t> palette1, palette2;
	std::vector<uint8_t> scratch, palettedImageScratch;
	LZ77Compressor compressor;

	void clear() {
		palette1.clear();
		palette2.clear();
		scratch.clear();
		palettedImageScratch.clear();
	}

	std::vector<uint8_t>& writePaletted(const Image& image, const std::unordered_map<Color, uint8_t>& palette, bool separateAlpha) {
		palettedImageScratch.clear();
		palettedImageScratch.resize(1024 + image.size.area() * (separateAlpha ? 2 : 1));
		Color* palData = reinterpret_cast<Color*>(palettedImageScratch.data());
		for (auto& pair : palette) {
			palData[pair.second] = pair.first;
		}
		uint8_t* ptr = palettedImageScratch.data() + 1024;
		for (Color c : image.colorData) {
			*ptr = palette.at(c);
			ptr++;
		}
		if (separateAlpha) {
			for (Color c : image.colorData) {
				*ptr = c.a;
				ptr++;
			}
		}

		return palettedImageScratch;
	}

	/// Calculate possible palettes for the image, p2 is for separate alpha
	void getPalettes(const Image& image, int& p1, int& p2) {
		palette1.clear();
		palette2.clear();
		p1 = p2 = 0;
		for (const Color& c : image.colorData) {
			Color p2c = c;
			p2c.a = 0;
			if (p2 >= 0 && palette2.find(p2c) == palette2.end()) {
				if (p2 == 256) {
					p2 = -1;
				} else {
					palette2[p2c] = p2;
					p2++;
				}
			}
			if (p1 >= 0 && palette1.find(c) == palette1.end()) {
				if (p1 == 256) {
					p1 = -1;
				} else {
					palette1[c] = p1;
					p1++;
				}
			}
		}
	}
};

Compressor::Compressor() {
	CompressorImpl* i = new CompressorImpl();
	i->compressor.configure(4, 12);
	impl = i;
}
Compressor::~Compressor() {
	delete static_cast<CompressorImpl*>(impl);
}
void Compressor::compress(std::vector<uint8_t>& output, const uint8_t* input, int inputLength, bool isSwitch) {
	LZ77Compressor::DefaultEncode enc { isSwitch };
	static_cast<CompressorImpl*>(impl)->compressor.compress(output, input, inputLength, enc);
}
ChunkHeader Compressor::encodeChunk(std::vector<uint8_t>& output, const Image& input, MaskRect bounds, Point location, bool isSwitch) {
	Image sized = input.resizeClampToEdge(align(input.size));
	if (!isSwitch) {
		swapBR(sized);
	}
	CompressorImpl* compressor = static_cast<CompressorImpl*>(impl);
	compressor->clear();
	ChunkHeader header = {};
	header.x = location.x;
	header.y = location.y;
	header.w = input.size.width;
	header.h = input.size.height;
	header.size = 0;
	bool hasTransparent = std::any_of(input.colorData.begin(), input.colorData.end(), [](Color c){ return c.a != 255; });
	if (hasTransparent) {
		header.transparentMasks.push_back(bounds);
	} else {
		header.masks.push_back(bounds);
	}
	int p1, p2;
	compressor->getPalettes(sized, p1, p2);
	if (p1 >= 0) {
		auto& img = compressor->writePaletted(sized, compressor->palette1, false);
		compress(output, img.data(), static_cast<int>(img.size()), isSwitch);
		if (img.size() < output.size()) {
			output = img;
		} else {
			header.size = output.size();
		}
		header.type = ChunkHeader::TYPE_INDEXED;
	}
	if (p2 >= 0 && hasTransparent) {
		auto& img = compressor->writePaletted(sized, compressor->palette2, true);
		compress(compressor->scratch, img.data(), static_cast<int>(img.size()), false);
		if (output.empty() || output.size() > compressor->scratch.size()) {
			output = compressor->scratch;
			header.size = output.size();
			header.type = ChunkHeader::TYPE_INDEXED_ALPHA;
		}
	}

	if (output.empty()) {
		prepareWriteRGB(compressor->scratch, sized);
		compress(output, compressor->scratch.data(), compressor->scratch.size(), isSwitch);
		header.size = output.size();
		header.type = ChunkHeader::TYPE_COLOR;
	}

	return header;
}
