#pragma once

#include <stdint.h>
#include <vector>
#include "Image.hpp"

bool decompressHigu(std::vector<uint8_t> &output, const uint8_t *input, int inputLength, bool isSwitch);

bool getIndexed(Image &output, const std::vector<uint8_t> &input, Size size, bool isSwitch, int colors = 256);

void getRGB(Image &image, const std::vector<uint8_t> &data, bool isSwitch);

void processChunkNoHeader(Image &output, uint32_t offset, uint32_t size, int indexed, int width, int height, std::istream &file, const std::string &name, bool isSwitch);

Point processChunk(Image &output, std::vector<MaskRect> &outputMasks, uint32_t offset, std::istream &file, const std::string &name, bool isSwitch);

void debugDecompress(uint32_t offset, uint32_t size, std::ifstream &in, bool isSwitch);
