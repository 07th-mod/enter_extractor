#include "FileTypes.hpp"

#include <stdint.h>
#include <vector>
#include <fstream>
#include <boost/filesystem.hpp>
#if ENABLE_MULTITHREADED
#include <boost/thread/thread_pool.hpp>
#endif

#include "Config.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"

int processTxa(std::ifstream &in, const boost::filesystem::path &output) {
	boost::filesystem::path outputDir = output.parent_path();
	std::string outTemplate = output.stem().string();

	TxaHeader header;
	in >> header;

#if ENABLE_MULTITHREADED
	boost::basic_thread_pool threadPool;
#endif

	for (const auto& chunk : header.chunks) {
		std::string outName = outTemplate + "_" + chunk.name;
		Image currentChunk({0, 0});
		processChunkNoHeader(currentChunk, chunk.offset, chunk.length, header.indexed, chunk.width, chunk.height, in, outName, header.isSwitch);

		auto outFilename = outputDir/(outName + ".png");
#if ENABLE_MULTITHREADED
		threadPool.submit([currentChunk = std::move(currentChunk), outFilename = std::move(outFilename)](){
			currentChunk.writePNG(outFilename);
		});
#else
		currentChunk.writePNG(outFilename);
#endif
	}

#if ENABLE_MULTITHREADED
	threadPool.close();
	threadPool.join();
#endif
	return 0;
}
