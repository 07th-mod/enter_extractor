#include "FileTypes.hpp"

#include <stdint.h>
#if ENABLE_MULTITHREADED
#include <boost/thread/thread_pool.hpp>
#endif

#include "Config.hpp"
#include "FS.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"
#include "Image.hpp"

int processBup(std::istream &in, const fs::path &output) {
	fs::path outputDir = output.parent_path();
	std::string outTemplate = output.stem().string();

	BupHeader header;
	in >> header;

	Image base({header.width, header.height}), currentChunk({0, 0});
	std::vector<MaskRect> maskData;

	for (const auto& chunk : header.chunks) {
		size_t i = &chunk - &header.chunks[0];

		Point pos = processChunk(currentChunk, maskData, chunk.offset, in, outTemplate + "_BaseChunk" + std::to_string(i), header.isSwitch);

		currentChunk.drawOnto(base, pos, maskData);
	}
	if (SHOULD_WRITE_DEBUG_IMAGES) {
		base.writePNG(debugImagePath/(outTemplate + "_Base.png"));
	}

	Image withEyes({0, 0});
#if ENABLE_MULTITHREADED
	boost::basic_thread_pool threadPool;
#endif

	for (const auto& expChunk : header.expChunks) {
		withEyes = base;

		if (expChunk.face.offset) {
			Point facePos = processChunk(currentChunk, maskData, expChunk.face.offset, in, outTemplate + "_" + expChunk.name + "_Face", header.isSwitch);
			currentChunk.drawOnto(withEyes, facePos, maskData);
		}

		bool atLeastOneMouth = false;
		for (int i = 0; i < expChunk.mouths.size(); i++) {
			const auto& mouth = expChunk.mouths[i];

			if (!mouth.offset) { continue; }
			atLeastOneMouth = true;

			Image withMouth = withEyes;

			Point mouthPos = processChunk(currentChunk, maskData, mouth.offset, in, outTemplate + "_" + expChunk.name + "_Mouth" + std::to_string(i), header.isSwitch);
			currentChunk.drawOnto(withMouth, mouthPos, maskData);
			auto outFilename = outputDir/(outTemplate + "_" + expChunk.name + "_" + std::to_string(i) + ".png");
#if ENABLE_MULTITHREADED
			threadPool.submit([withMouth = std::move(withMouth), outFilename = std::move(outFilename)](){
				withMouth.writePNG(outFilename);
			});
#else
			withMouth.writePNG(outFilename);
#endif
		}
		if (!atLeastOneMouth) {
			auto outFilename = outputDir/(outTemplate + "_" + expChunk.name + ".png");
#if ENABLE_MULTITHREADED
			threadPool.submit([withEyes = std::move(withEyes), outFilename = std::move(outFilename)]{
				withEyes.writePNG(outFilename);
			});
#else
			withEyes.writePNG(outFilename);
#endif
		}
	}

#if ENABLE_MULTITHREADED
	threadPool.close();
	threadPool.join();
#endif

	return 0;
}
