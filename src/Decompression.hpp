#pragma once

#include <stdint.h>
#include <vector>
#include "Image.hpp"

bool decompressHigu(std::vector<uint8_t> &output, const uint8_t *input, int inputLength);

bool getIndexed(Image &output, const std::vector<uint8_t> &input, Size size, int colors = 256);

void getRGB(Image &image, const std::vector<uint8_t> &data);

void processChunkNoHeader(Image &output, uint32_t offset, uint32_t size, int indexed, int width, int height, std::ifstream &file, const std::string &name);

Point processChunk(Image &output, std::vector<MaskRect> &outputMasks, uint32_t offset, std::ifstream &file, const std::string &name);

void debugDecompress(uint32_t offset, uint32_t size, std::ifstream &in);
