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
//#include "RegionChecker.hpp"

#if SHOULD_TEMPLATE
template <typename BupHeader>
#else
inline
#endif
int processBup(std::ifstream &in, const boost::filesystem::path &output) {
	in.seekg(0, in.end);
//	RegionChecker rc(in.tellg());
	in.seekg(0, in.beg);

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

//	rc.add(0, in.tellg());

	Image base({header.width, header.height}), currentChunk({0, 0});

	for (int i = 0; i < header.baseChunks; i++) {
		const auto& chunk = chunks[i];

		auto info = processChunk(currentChunk, chunk.offset, in);
		bool masked = info.first;
		Point pos = info.second;

//		rc.add(chunk.offset, chunk.offset + chunk.getSize());

		if (SHOULD_WRITE_DEBUG_IMAGES) {
			std::string outFilename = outTemplate + "_BaseChunk" + std::to_string(i) + (masked ? "_masked.png" : ".png");
			currentChunk.writePNG(debugImagePath/outFilename);
		}

		currentChunk.drawOnto(base, pos, currentChunk.size);
	}
	if (SHOULD_WRITE_DEBUG_IMAGES) {
		base.writePNG(debugImagePath/(outTemplate + "_Base.png"));
	}

	boost::locale::generator gen;
	std::locale cp932 = gen.generate("ja_JP.cp932");
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
			std::cerr << "Expression " << (&expChunk - &expChunks[0]) << ", " << name << ", had nonzero values in its expression data..." << std::endl;
		}

		if (!expChunk.face.offset) {
			base.writePNG(outputDir/(outTemplate + "_" + name + ".png"));
			continue;
		}

		withEyes = base;

//		rc.add(expChunk.face.offset, expChunk.face.offset + expChunk.face.getSize());
		auto faceInfo = processChunk(currentChunk, expChunk.face.offset, in);
		if (SHOULD_WRITE_DEBUG_IMAGES) {
			std::string outFilename = outTemplate + "_Face_" + name + (faceInfo.first ? "_masked.png" : ".png");
			currentChunk.writePNG(debugImagePath/outFilename);
		}
		currentChunk.drawOntoCombine(withEyes, faceInfo.second, currentChunk.size, header.combineMode);

		bool atLeastOneMouth = false;
		for (int i = 0; i < expChunk.mouths.size(); i++) {
			const auto& mouth = expChunk.mouths[i];

			if (!mouth.offset) {
				continue;
			}
			atLeastOneMouth = true;

//			rc.add(mouth.offset, mouth.offset + mouth.getSize());

			std::unique_ptr<Image> withMouth(new Image(withEyes));

			auto mouthInfo = processChunk(currentChunk, mouth.offset, in);
			if (SHOULD_WRITE_DEBUG_IMAGES) {
				std::string outFilename = outTemplate + "_Mouth" + std::to_string(i) + "_" + name + (mouthInfo.first ? "_masked.png" : ".png");
				currentChunk.writePNG(debugImagePath/outFilename);
			}
			currentChunk.drawOntoCombine(*withMouth, mouthInfo.second, currentChunk.size, header.combineMode);
			auto outFilename = outputDir/(outTemplate + "_" + name + "_" + std::to_string(i) + ".png");
			#if ENABLE_MULTITHREADED
			threadPool.submit([withMouth = std::move(withMouth), outFilename = std::move(outFilename)](){
				withMouth->writePNG(outFilename);
			});
			#else
			withMouth->writePNG(outFilename);
			#endif
		}
		if (!atLeastOneMouth) {
			withEyes.writePNG(outputDir/(outTemplate + "_" + name + ".png"));
		}
	}

	#if ENABLE_MULTITHREADED
	threadPool.join();
	#endif

//	rc.printRegions();
	return 0;
}
