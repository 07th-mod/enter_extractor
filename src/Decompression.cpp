#include "Decompression.hpp"

#include <fstream>
#include <iostream>
#include <cassert>
#include "HeaderStructs.hpp"

// I have no idea why
static int adjustW(int w) {
	return (w + 3) / 4 * 4;
}

bool decompressHigu(std::vector<uint8_t> &output, const uint8_t *input, int inputLength) {
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

			b1 = (b1 >> 4) | (b1 << 4);

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

void getRGB(Image &image, const std::vector<uint8_t> &data) {
	int scanline = sizeof(Color) * image.size.width;
	uint8_t *start = (uint8_t *)image.colorData.data();
	memcpy(start, data.data(), scanline);
	for (int i = scanline; i < sizeof(Color) * image.colorData.size(); i++) {
		start[i] = start[i - scanline] + data[i];
	}
}

bool checkMask(const std::vector<uint8_t> &input, Size size) {
	int maskStart = 1024 + size.area();
	bool all = true;
	for (int i = maskStart; i < size.area(); i++) {
		if (input[i] != 255) {
			all = false;
		}
	}
	return all;
}

uint8_t vecGet(std::vector<uint8_t> vec, int pos) {
	return vec[pos];
}

bool getIndexed(Image &output, const std::vector<uint8_t> &input, Size size, int colors, bool crop) {
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

	if (crop) {
		output = output.resized({size.width - 2, size.height - 2});
	}

	return maskStart == input.size();
}

std::pair<bool, Point> processChunk(Image &output, uint32_t offset, std::ifstream &file) {
	file.seekg(offset, file.beg);
	ChunkHeader header;
	file.read((char *)&header, sizeof(header));

	// Who knows what's going on here
	bool weirdMagicFlag = ((header.type == 0 || header.type == 2) && (header.unk0 > 0 || header.unk5 != 0))
	                   || ((header.type == 1 || header.type == 3) && (header.unk0 > 0 && header.unk5 != 0));

	std::vector<uint8_t> decompressed;

	Size outputSize = {adjustW(header.w), header.h};

	// If size is zero, then we're uncompressed and paletted
	if (header.size == 0) {
		if (header.type == 3 || header.type == 2) {
			if (weirdMagicFlag) {
				uint32_t flag;
				do {
					file.ignore(12);
					file.read((char *)&flag, sizeof(flag));
				} while (flag != 0);
			}

			// 1024 for the palette, plus one for each pixel.
			int size = 1024 + outputSize.area();

			std::vector<uint8_t> chunk(size);
			file.read((char *)chunk.data(), chunk.size());
			bool masked = getIndexed(output, chunk, outputSize);
			return {masked, {header.x, header.y}};
		}
		output.fastResize({0, 0});
		return {false, {0, 0}};
	}
	else if (weirdMagicFlag) {
		std::vector<uint8_t> compressed(header.size);
		file.read((char *)compressed.data(), compressed.size());
		int offset = 0;
		while (true) {
			if (decompressHigu(decompressed, compressed.data() + offset, (int)compressed.size() - offset)) {
				if (
					decompressed.size() == 1024 + outputSize.area() ||     // Indexed, no mask
					decompressed.size() == 1024 + outputSize.area() * 2 || // Indexed, with mask
					decompressed.size() == outputSize.area() * 4           // Not indexed
				) {
					break;
				}
			}
			do {
				int amount = 12 + sizeof(int);
				size_t old = compressed.size();
				offset += amount;
				compressed.resize(old + amount);
				file.read((char *)compressed.data() + old, amount);
			} while (*(int *)&compressed[offset - sizeof(int)] != 0);
		}
	}
	else {
		std::vector<uint8_t> compressed(header.size);
		file.read((char *)compressed.data(), compressed.size());
		if (!decompressHigu(decompressed, compressed.data(), (int)compressed.size())) {
			throw std::runtime_error("Decompression of chunk failed when we expected it to succeed");
		}
	}

	bool masked;
	if (header.type == 3 || header.type == 2) {
		masked = getIndexed(output, decompressed, outputSize);
	}
	else {
		output.fastResize(outputSize);
		int byteSize = output.size.area() * sizeof(Color);
		if (decompressed.size() < byteSize) {
			fprintf(stderr, "Decompressed too little data for chunk, have %ld but need %d bytes!\n", decompressed.size(), byteSize);
			decompressed.resize(byteSize);
		}
		else if (decompressed.size() > byteSize) {
			fprintf(stderr, "Decompressed too much data for chunk, have %ld but only need %d bytes!\n", decompressed.size(), byteSize);
		}
		getRGB(output, decompressed);

		masked = true;
	}

	return {masked, {header.x, header.y}};
}

void debugDecompress(uint32_t offset, uint32_t size, std::ifstream &in) {
	std::vector<uint8_t> data(size);
	std::vector<uint8_t> output;
	in.seekg(offset, in.beg);
	ChunkHeader header;
	in.read((char *)&header, sizeof(header));
	in.seekg(offset, in.beg);
	in.read((char *)data.data(), data.size());

	for (int i = 0; i < data.size(); i++) {
		if (decompressHigu(output, data.data() + i, (int)data.size() - i)) {
			std::ofstream out("/tmp/chunks/data" + std::to_string(i) + ".dat");
			out.write((char *)output.data(), output.size());
			if (output.size() >= 1024 + header.w * header.h) {
				Image img({0, 0});
				getIndexed(img, output, {adjustW(header.w), header.h});
				img.writePNG("/tmp/chunks/data" + std::to_string(i) + ".png");
			}
		}
	}
}