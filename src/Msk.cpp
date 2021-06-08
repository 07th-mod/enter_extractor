#include "FileTypes.hpp"

#include <stdint.h>

#include "Config.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"

int processMsk3(std::istream &in, const fs::path &output) {
	Msk3Header header;
	in >> header;
	Size size { header.width, header.height };

	std::vector<uint8_t> compressedData, decompressedData;
	compressedData.resize(header.compressedSize);
	in.read((char *)compressedData.data(), compressedData.size());

	if (!decompressHigu(decompressedData, compressedData.data(), (int)compressedData.size(), header.isSwitch)) {
		throw std::runtime_error("Decompression failed");
	}

	if (decompressedData.size() != size.area()) {
		throw std::runtime_error("Expected " + std::to_string(size.area()) + " bytes but got " + std::to_string(decompressedData.size()) + " bytes");
	}

	writePNG(output, PNGColorType::GRAY, size, decompressedData.data());

	return 0;
}

int processMsk4(std::istream &in, const fs::path &output) {
	Msk4Header header;
	in >> header;
	Size size { header.width, header.height };

	std::vector<uint8_t> compressedData, decompressedData;
	// First thing in data is its size (uint32)
	compressedData.resize(header.dataSize - 4);
	in.seekg(header.dataOffset + 4, in.beg);
	in.read((char *)compressedData.data(), compressedData.size());

	// TODO: Figure out what the stuff between the header and data is

	if (!decompressHigu(decompressedData, compressedData.data(), (int)compressedData.size(), true)) {
		throw std::runtime_error("Decompression failed");
	}

	if (decompressedData.size() != size.area()) {
		throw std::runtime_error("Expected " + std::to_string(size.area()) + " bytes but got " + std::to_string(decompressedData.size()) + " bytes when processing " + currentFileName.string());
	}

	writePNG(output, PNGColorType::GRAY, size, decompressedData.data());

	return 0;
}
