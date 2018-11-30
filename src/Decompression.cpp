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

	return maskStart != input.size();
}

static void printDebugAndWrite(const Image &currentOutput, const ChunkHeader &header, bool masked, int skipStart, int skipLen, const std::string &name, std::ifstream &file) {
	std::cout << name << std::endl;
	std::cout << "  Type " << header.type << " " << header.unk0 << std::endl;
	std::cout << "  UNK  " << (header.unk1 & 0xFF) << " " << (header.unk1 >> 16) << std::endl;
	std::cout << "  X Y  " << (header.x) << " " << (header.y) << std::endl;
	std::cout << "  W H  " << (header.w) << " " << (header.h) << std::endl;
	std::cout << "  UNK  " << (header.unk3 & 0xFF) << " " << (header.unk3 >> 16) << std::endl;
	std::cout << "  UNK  " << (header.unk4 & 0xFF) << " " << (header.unk4 >> 16) << std::endl;
	std::cout << "  UNK  " << (header.unk5 & 0xFF) << " " << (header.unk5 >> 16) << std::endl;
	std::cout << "  Extra " << skipLen << " at " << skipStart << std::endl;

	currentOutput.writePNG(debugImagePath/(name + (masked ? "_masked.png" : ".png")));

	std::ofstream outFile("/tmp/chunks/" + name + ".svg");
	outFile << "<svg version=\"1.1\" baseProfile=\"full\" height=\"" << header.h << "\" width=\"" << header.w << "\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";
	outFile << "<image xlink:href=\"" << name << (masked ? "_masked.png" : ".png") << "\" x=\"0\" y=\"-1\" height=\"" << header.h << "\" width=\"" << header.w << "\"/>\n";
	std::vector<uint16_t> a = std::vector<uint16_t>(6 + skipLen / 2);
	file.seekg(skipStart);
	file.read((char *)a.data() + 12, skipLen);
	memcpy(a.data(), &header.unk3, 12);
	for (int i = 0; i + 3 < 6 + (skipLen / 2); i += 4) {
		Point start = { .x = a[i], .y = a[i+1] };
		Point end = { .x = a[i+2], .y = a[i+3] };
		if (start.x == end.x && start.y == end.y) { break; }
		outFile << "<rect x=\"" << start.x << "\" y=\"" << start.y << "\" width=\"" << (end.x-start.x) << "\" height=\"" << (end.y-start.y) << "\" style=\"fill:red;\" />" << std::endl;
	}
	outFile << "</svg>";
}

std::pair<bool, Point> processChunk(Image &output, uint32_t offset, std::ifstream &file, const std::string &name) {
	file.seekg(offset, file.beg);
	ChunkHeader header;
	file.read((char *)&header, sizeof(header));

	// Who knows what's going on here
	bool weirdMagicFlag = ((header.type == 0 || header.type == 2) && (header.unk0 > 0 || header.unk5 != 0))
	                   || ((header.type == 1 || header.type == 3) && (header.unk0 > 0 && header.unk5 != 0));

	std::vector<uint8_t> decompressed;

	Size outputSize = {adjustW(header.w), header.h};
	int skipStart = 0;
	int skipLen = 0;
	// If size is zero, then we're uncompressed and paletted
	if (header.size == 0) {
		if (header.type == 3 || header.type == 2) {
			if (weirdMagicFlag) {
				uint32_t flag;
				skipStart = file.tellg();
				do {
					file.ignore(12);
					file.read((char *)&flag, sizeof(flag));
				} while (flag != 0);
				skipLen = (int)file.tellg() - skipStart;
			}

			// 1024 for the palette, plus one for each pixel.
			int size = 1024 + outputSize.area();

			std::vector<uint8_t> chunk(size);
			file.read((char *)chunk.data(), chunk.size());
			bool masked = getIndexed(output, chunk, outputSize, 256, false);
			if (SHOULD_WRITE_DEBUG_IMAGES) {
				printDebugAndWrite(output, header, masked, skipStart, skipLen, name, file);
			}
			output = output.resized({header.w - 2, header.h - 2});
			return {masked, {header.x, header.y}};
		}
		output.fastResize({0, 0});
		return {false, {0, 0}};
	}
	else if (weirdMagicFlag) {
		skipStart = file.tellg();
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
					if (offset > 0) {
						skipLen = offset;
					}
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
		masked = getIndexed(output, decompressed, outputSize, 256, false);
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

	if (SHOULD_WRITE_DEBUG_IMAGES) {
		printDebugAndWrite(output, header, masked, skipStart, skipLen, name, file);
	}

	output = output.resized({header.w - 2, header.h - 2});

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
