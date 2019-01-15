#include "RegionChecker.hpp"
#include <iostream>

RegionChecker::RegionChecker(std::istream &file) {
	file.seekg(0, file.beg);
	std::istreambuf_iterator<char> iter(file);
	while (iter != std::istreambuf_iterator<char>()) {
		fileUsage.push_back(*iter != 0);
		iter++;
	}
	file.seekg(0, file.beg);
	usedBits = std::vector<bool>(fileUsage.size());
}

void RegionChecker::add(size_t from, size_t to) {
	for (size_t i = from; i < to; i++) {
		usedBits[i] = true;
	}

	std::cout << "Added region [" << from << ", " << to << ")" << std::endl;
}

void RegionChecker::printRange(int from, int to) const {
	for (int i = from; i < to; i++) {
		if (fileUsage[i]) {
			goto nonzero;
		}
	}
	return;
nonzero:
	std::cout << "Size: " << (to - from) << " Start: " << from << " End: " << to << std::endl;
}

void RegionChecker::printRegions() const {
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
