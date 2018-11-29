#pragma once

#include <stdint.h>
#include <vector>

/// Checks which regions were actually used by the decoder
class RegionChecker {
public:
	RegionChecker(size_t size);

	void add(size_t from, size_t to);
	void printRegions();
private:
	std::vector<bool> usedBits;
};
