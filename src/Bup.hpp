#pragma once

#include <stdint.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#if ENABLE_MULTITHREADED
#include <boost/thread/thread_pool.hpp>
#endif

#include "Config.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"
#include "Image.hpp"

#if SHOULD_TEMPLATE
template <typename BupHeader>
#else
inline
#endif
int processBup(std::ifstream &in, const boost::filesystem::path &output) {
	boost::filesystem::path outputDir = output.parent_path();
	std::string outTemplate = output.stem().string();

	BupHeader header;
	in.read((char *)&header, sizeof(header));

	// Skip bytes for unknown reason
	in.ignore(header.tbl1 * header.skipAmount);

	std::vector<typename BupHeader::Chunk> chunks(header.baseChunks);
	std::vector<typename BupHeader::ExpressionChunk> expChunks(header.expChunks);
	in.read((char *)chunks.data(), chunks.size() * sizeof(chunks[0]));
	in.ignore(header.skipAmount2);
	in.read((char *)expChunks.data(), expChunks.size() * sizeof(expChunks[0]));

	Image base({header.width, header.height}), currentChunk({0, 0});
	std::vector<MaskRect> maskData;

	for (int i = 0; i < header.baseChunks; i++) {
		const auto& chunk = chunks[i];

		Point pos = processChunk(currentChunk, maskData, chunk.offset, in, outTemplate + "_BaseChunk" + std::to_string(i));

		currentChunk.drawOnto(base, pos, maskData);
	}
	if (SHOULD_WRITE_DEBUG_IMAGES) {
		base.writePNG(debugImagePath/(outTemplate + "_Base.png"));
	}

	Image withEyes({0, 0});
	#if ENABLE_MULTITHREADED
	boost::basic_thread_pool threadPool;
	#endif

	for (const auto& expChunk : expChunks) {
		std::string name = boost::locale::conv::to_utf<char>(expChunk.name, cp932);

		if (name.empty()) {
			name = "expression" + std::to_string(&expChunk - &expChunks[0]);
		}
		if (!expChunk.unkBytesValid()) {
			std::cerr << "Expression " << (&expChunk - &expChunks[0]) << ", " << name << ", had unexpected nonzero values in its expression data..." << std::endl;
		}

		withEyes = base;
		
		if (expChunk.face.offset) {
			Point facePos = processChunk(currentChunk, maskData, expChunk.face.offset, in, outTemplate + "_" + name + "_Face");
			currentChunk.drawOnto(withEyes, facePos, maskData);
		}

		bool atLeastOneMouth = false;
		for (int i = 0; i < expChunk.mouths.size(); i++) {
			const auto& mouth = expChunk.mouths[i];

			if (!mouth.offset) { continue; }
			atLeastOneMouth = true;

			Image withMouth = withEyes;

			Point mouthPos = processChunk(currentChunk, maskData, mouth.offset, in, outTemplate + "_" + name + "_Mouth" + std::to_string(i));
			currentChunk.drawOnto(withMouth, mouthPos, maskData);
			auto outFilename = outputDir/(outTemplate + "_" + name + "_" + std::to_string(i) + ".png");
			#if ENABLE_MULTITHREADED
			threadPool.submit([withMouth = std::move(withMouth), outFilename = std::move(outFilename)](){
				withMouth.writePNG(outFilename);
			});
			#else
			withMouth.writePNG(outFilename);
			#endif
		}
		if (!atLeastOneMouth) {
			withEyes.writePNG(outputDir/(outTemplate + "_" + name + ".png"));
		}
	}

	#if ENABLE_MULTITHREADED
	threadPool.join();
	#endif

	return 0;
}
