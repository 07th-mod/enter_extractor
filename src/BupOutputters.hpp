#pragma once

#include "FS.hpp"
#include "Image.hpp"

class BupOutputter {
public:
	static std::unique_ptr<BupOutputter> makeComposited();

	BupOutputter() {}
	virtual ~BupOutputter() {}
	virtual void setBase(fs::path basePath, Image img, std::string name) = 0;
	virtual void newFace(const Image& img, Point pos, const std::string& name, const std::vector<MaskRect>& mask) = 0;
	virtual void newMouth(const Image& img, Point pos, int num, const std::vector<MaskRect>& mask) = 0;
	virtual void write() = 0;
};
