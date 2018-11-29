#include "RegionChecker.hpp"
#include <iostream>

RegionChecker::RegionChecker(size_t size): usedBits(std::vector<bool>(size)) {}

void RegionChecker::add(size_t from, size_t to) {
	for (size_t i = from; i < to; i++) {
		usedBits[i] = true;
	}

	std::cout << "Added region [" << from << ", " << to << ")" << std::endl;
}

static void printRange(int from, int to) {
	std::cout << "Size: " << (to - from) << " Start: " << from << " End: " << to << std::endl;
}

void RegionChecker::printRegions() {
	bool inUnusedRegion = false;
	int startMarker = 0;
	for (int i = 0; i < usedBits.size(); i++) {
		if (!inUnusedRegion && usedBits[i] == false) {
			inUnusedRegion = true;
			startMarker = i;
		}
		else if (inUnusedRegion && usedBits[i] == true) {
			inUnusedRegion = false;
			printRange(startMarker, i);
		}
	}
	if (inUnusedRegion) {
		printRange(startMarker, usedBits.size());
	}
}
