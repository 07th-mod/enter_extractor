#pragma once

#include <stdint.h>
#include <vector>
#include <iostream>

/// Checks which regions were actually used by the decoder
class RegionChecker {
public:
	RegionChecker(std::istream &file);

	void add(size_t from, size_t to);
	void printRegions() const;
private:
	void printRange(int from, int to) const;
	std::vector<bool> usedBits;
	std::vector<bool> fileUsage;
};
