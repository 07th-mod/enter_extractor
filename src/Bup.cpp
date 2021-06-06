#include "FileTypes.hpp"

#include <stdint.h>

#include "BupOutputters.hpp"
#include "Config.hpp"
#include "FS.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"
#include "Image.hpp"

int processBup(std::istream &in, const fs::path &output) {
	fprintf(stderr, "ProcessBup started, output is %s\n", output.string().c_str());
	fs::path outputDir = output.parent_path();
	std::string outTemplate = output.stem().string();

	std::unique_ptr<BupOutputter> out = SAVE_BUP_AS_PARTS ? BupOutputter::makeParts() : BupOutputter::makeComposited();

	BupHeader header;
	in >> header;

	Image base({header.width, header.height}), currentChunk({0, 0});
	std::vector<MaskRect> maskData;
	fprintf(stderr, "Will process bup to %s\n", output.string().c_str());

	for (const auto& chunk : header.chunks) {
		size_t i = &chunk - &header.chunks[0];

		Point pos = processChunk(currentChunk, maskData, chunk.offset, in, outTemplate + "_BaseChunk" + std::to_string(i), header.isSwitch);

		currentChunk.drawOnto(base, pos, maskData);
	}
	if (SHOULD_WRITE_DEBUG_IMAGES) {
		base.writePNG(debugImagePath/(outTemplate + "_Base.png"));
	}
	
	fprintf(stderr, "Processed base image\n");

	out->setBase(outputDir, std::move(base), outTemplate);

	Image withEyes({0, 0});

	for (const auto& expChunk : header.expChunks) {
		withEyes = base;

		if (expChunk.face.offset) {
			Point facePos = processChunk(currentChunk, maskData, expChunk.face.offset, in, outTemplate + "_" + expChunk.name + "_Face", header.isSwitch);
			out->newFace(currentChunk, facePos, expChunk.name, maskData);
		} else {
			out->newFace(Image(), {0, 0}, "", {});
		}
		
		fprintf(stderr, "ExpChunk\n");

		bool atLeastOneMouth = false;
		for (int i = 0; i < expChunk.mouths.size(); i++) {
			const auto& mouth = expChunk.mouths[i];

			if (!mouth.offset) { continue; }
			atLeastOneMouth = true;

			Image withMouth = withEyes;

			Point mouthPos = processChunk(currentChunk, maskData, mouth.offset, in, outTemplate + "_" + expChunk.name + "_Mouth" + std::to_string(i), header.isSwitch);
			out->newMouth(currentChunk, mouthPos, i, maskData);
			fprintf(stderr, "Writing image...\n");
			out->write();
		}
		if (!atLeastOneMouth) {
			out->write();
		}
	}

	return 0;
}
