#include "Decompression.hpp"

#include <fstream>
#include <iostream>
#include <cassert>
#include "HeaderStructs.hpp"

static int adjustW(int w) {
	return (w + 3) & ~3;
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

static void printDebugAndWrite(const Image &currentOutput, const ChunkHeader &header, const std::vector<MaskRect> &maskData, const std::string &name, std::ifstream &file) {
	std::cout << "========== " << name << " ==========" << std::endl;
	std::cout << "               Type " << header.type << std::endl;
	std::cout << "              Masks " << header.masks << std::endl;
	std::cout << "  Transparent Masks " << header.transparentMasks << std::endl;
	std::cout << "    Alignment Words " << header.alignmentWords << std::endl;
	std::cout << "                X Y " << header.x << " " << header.y << std::endl;
	std::cout << "                W H " << header.w << " " << header.h << std::endl;
	std::cout << "               Size " << header.size << std::endl;

	currentOutput.writePNG(debugImagePath/(name + ".png"));
	Image masked(currentOutput.size);
	currentOutput.drawOnto(masked, {0, 0}, maskData);
	masked.writePNG(debugImagePath/(name + "_masked.png"));
}

void processChunkNoHeader(Image &output, uint32_t offset, uint32_t size, int indexed, int width, int height, std::ifstream &file, const std::string &name, bool isSwitch) {
	file.seekg(offset);

	std::vector<uint8_t> decompressed;
	Size alignedSize = {(width + 3) & ~3, height};

	// If size is zero, then we're uncompressed and paletted
	if (size == 0) {
		if (!indexed) {
			throw std::runtime_error("Expected a size-0 type to be indexed but it wasn't...");
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

	if (indexed) {
		getIndexed(output, decompressed, alignedSize, isSwitch);
	}
	else {
		output.fastResize(alignedSize);
		int byteSize = output.size.area() * sizeof(Color);
		if (decompressed.size() < byteSize) {
			fprintf(stderr, "Decompressed too little data for chunk, have %ld but need %d bytes!\n", decompressed.size(), byteSize);
			decompressed.resize(byteSize);
		}
		else if (decompressed.size() > byteSize) {
			fprintf(stderr, "Decompressed too much data for chunk, have %ld but only need %d bytes!\n", decompressed.size(), byteSize);
		}
		getRGB(output, decompressed, isSwitch);
	}
}

Point processChunk(Image &output, std::vector<MaskRect> &outputMasks, uint32_t offset, std::ifstream &file, const std::string &name, bool isSwitch) {
	file.seekg(offset, file.beg);
	ChunkHeader header;
	file >> header;

	outputMasks.resize(header.masks + header.transparentMasks);
	size_t masksSize = sizeof(outputMasks[0]) * outputMasks.size();
	file.read((char *)outputMasks.data(), masksSize);
	file.ignore(header.alignmentWords * 2);

	std::vector<uint8_t> decompressed;

	Size alignedSize = {(header.w + 3) & ~3, header.h};

	// If size is zero, then we're uncompressed and paletted
	if (header.size == 0) {
		// 1024 for the palette, plus one for each pixel.
		int size = 1024 + alignedSize.area();

		std::vector<uint8_t> chunk(size);
		file.read((char *)chunk.data(), chunk.size());
		getIndexed(output, chunk, alignedSize, isSwitch);
		if (SHOULD_WRITE_DEBUG_IMAGES) {
			printDebugAndWrite(output, header, outputMasks, name, file);
		}
		return {header.x, header.y};
	}
	else {
		std::vector<uint8_t> compressed(header.size);
		file.read((char *)compressed.data(), compressed.size());
		if (!decompressHigu(decompressed, compressed.data(), (int)compressed.size(), isSwitch)) {
			throw std::runtime_error("Decompression of " + name + " failed");
		}
	}

	if (header.type == 3 || header.type == 2) {
		getIndexed(output, decompressed, alignedSize, isSwitch);
	}
	else {
		output.fastResize(alignedSize);
		int byteSize = output.size.area() * sizeof(Color);
		if (decompressed.size() < byteSize) {
			fprintf(stderr, "Decompressed too little data for chunk, have %ld but need %d bytes!\n", decompressed.size(), byteSize);
			decompressed.resize(byteSize);
		}
		else if (decompressed.size() > byteSize) {
			fprintf(stderr, "Decompressed too much data for chunk, have %ld but only need %d bytes!\n", decompressed.size(), byteSize);
		}
		getRGB(output, decompressed, isSwitch);
	}

	if (SHOULD_WRITE_DEBUG_IMAGES) {
		printDebugAndWrite(output, header, outputMasks, name, file);
	}

	return {header.x, header.y};
}

void debugDecompress(uint32_t offset, uint32_t size, std::ifstream &in, bool isSwitch) {
	std::vector<uint8_t> data(size);
	std::vector<uint8_t> output;
	in.seekg(offset, in.beg);
	ChunkHeader header;
	in >> header;
	in.seekg(offset, in.beg);
	in.read((char *)data.data(), data.size());

	for (int i = 0; i < data.size(); i++) {
		if (decompressHigu(output, data.data() + i, (int)data.size() - i, isSwitch)) {
			std::ofstream out("/tmp/chunks/data" + std::to_string(i) + ".dat");
			out.write((char *)output.data(), output.size());
			if (output.size() >= 1024 + header.w * header.h) {
				Image img({0, 0});
				getIndexed(img, output, {adjustW(header.w), header.h}, isSwitch);
				img.writePNG("/tmp/chunks/data" + std::to_string(i) + ".png");
			}
		}
	}
}
