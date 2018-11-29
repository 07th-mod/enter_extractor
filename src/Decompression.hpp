#pragma once

#include <stdint.h>
#include <vector>
#include "Image.hpp"

bool decompressHigu(std::vector<uint8_t> &output, const uint8_t *input, int inputLength);

bool getIndexed(Image &output, const std::vector<uint8_t> &input, Size size, int colors = 256, bool crop = true);

void getRGB(Image &image, const std::vector<uint8_t> &data);

std::pair<bool, Point> processChunk(Image &output, uint32_t offset, std::ifstream &file, const std::string &name);

void debugDecompress(uint32_t offset, uint32_t size, std::ifstream &in);
