#pragma once

#include <stdint.h>
#include <fstream>
#include <boost/filesystem.hpp>

#include "Config.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"

#if SHOULD_TEMPLATE
template <typename Msk3Header>
#else
inline
#endif
int processMsk3(std::ifstream &in, const boost::filesystem::path &output) {
	Msk3Header header;
	in.read((char *)&header, sizeof(header));
	Size size { header.width, header.height };

	std::vector<uint8_t> compressedData, decompressedData;
	compressedData.resize(header.compressedSize);
	in.read((char *)compressedData.data(), compressedData.size());

	if (!decompressHigu(decompressedData, compressedData.data(), (int)compressedData.size(), Msk3Header::IsSwitch::value)) {
		throw std::runtime_error("Decompression failed");
	}

	if (decompressedData.size() != size.area()) {
		throw std::runtime_error("Expected " + std::to_string(size.area()) + " bytes but got " + std::to_string(decompressedData.size()) + " bytes");
	}

	writePNG(output, PNGColorType::GRAY, size, decompressedData.data());

	return 0;
}
