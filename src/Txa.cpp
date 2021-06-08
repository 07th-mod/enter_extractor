#include "FileTypes.hpp"

#include <stdint.h>
#include "Config.hpp"
#include "FS.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"
#include "Utilities.hpp"

int processTxa(std::istream &in, const fs::path &output) {
	fs::path outputDir = output.parent_path();
	std::string outTemplate = output.stem().string();

	TxaHeader header;
	in >> header;

#if ENABLE_MULTITHREADED
	ThreadedImageSaver saver;
#endif

	for (const auto& chunk : header.chunks) {
		std::string outName = outTemplate + "_" + chunk.name;
		Image currentChunk({0, 0});
		processChunkNoHeader(currentChunk, chunk.offset, chunk.length, header.indexed, chunk.width, chunk.height, in, outName, header.isSwitch);

		auto outFilename = outputDir/fs::u8path(outName + ".png");
#if ENABLE_MULTITHREADED
		saver.enqueue(std::move(currentChunk), std::move(outFilename));
#else
		currentChunk.writePNG(outFilename);
#endif
	}
	return 0;
}

int replaceTxa(std::istream &in, std::ostream &output, const fs::path &replacement) {
	fs::path replacementDir = replacement.parent_path();
	std::string replacementTemplate = replacement.stem().string();

	TxaHeader header;
	in >> header;

	std::vector<Image> images(header.chunks.size(), Image());
	std::vector<std::vector<uint8_t>> chunks(header.chunks.size(), std::vector<uint8_t>());
	Compressor compressor;

	bool indexed = true;

	for (size_t i = 0; i < chunks.size(); i++) {
		std::string replacementName = replacementTemplate + "_" + header.chunks[i].name;
		auto rfilename = replacementDir/fs::u8path(replacementName + ".png");

		try {
			images[i] = Image::readPNG(rfilename);
		} catch (std::runtime_error&) {
			fprintf(stderr, "Failed to load replacement %s, not replacing\n", rfilename.string().c_str());
			const auto& chunk = header.chunks[i];
			processChunkNoHeader(images[i], chunk.offset, chunk.length, header.indexed, chunk.width, chunk.height, in, replacementName, header.isSwitch);
		}

		if (!compressor.canPalette(images[i], false)) {
			fprintf(stderr, "%s has too many colors to palette, disabling indexed color for all images\n", replacementName.c_str());
			indexed = false;
		}
	}

	header.indexed = indexed;

	parallel_for(0, chunks.size(), []{ return Compressor(); }, [&](Compressor& c, size_t i) {
		ChunkHeader::Type type = indexed ? ChunkHeader::TYPE_INDEXED : ChunkHeader::TYPE_COLOR;
		if (!c.encodeHeaderlessChunk(chunks[i], images[i], type, header.isSwitch)) {
			throw std::runtime_error("Failed to encode chunk " + std::to_string(i));
		}
		auto& h = header.chunks[i];
		h.decodedLength = indexed ? (1024 + images[i].size.area()) : (4 * images[i].size.area());
		h.width = images[i].size.width;
		h.height = images[i].size.height;
		h.length = chunks[i].size();
	});

	int pos = header.updateAndCalcBinSize();
	for (auto& chunk : header.chunks) {
		pos = align(pos, 16);
		chunk.offset = pos;
		pos += chunk.length;
	}

	header.filesize = pos;
	header.write(output, in);
	for (size_t i = 0; i < chunks.size(); i++) {
		output.seekp(header.chunks[i].offset, output.beg);
		output.write(reinterpret_cast<const char*>(chunks[i].data()), chunks[i].size());
	}
	return 0;
}
