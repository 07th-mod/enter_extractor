#include "BupOutputters.hpp"

#include "Utilities.hpp"

class CompositedBupOutputter: public BupOutputter {
	fs::path basePath;
	Image base;
	std::string baseName;
	Image withFace;
	std::string withFaceName;
	Image withMouth;
	std::string withMouthName;
public:
	void setBase(fs::path basePath, Image img, std::string name) override {
		this->basePath = std::move(basePath);
		base = std::move(img);
		baseName = std::move(name);
		withFaceName = "";
		withMouthName = "";
	}
	void newFace(const Image& img, Point pos, const std::string& name, const std::vector<MaskRect>& mask) override {
		withFace = base;
		withMouthName = "";
		if (img.empty()) {
			withFaceName = baseName;
		} else {
			withFaceName = baseName + "_" + name;
			img.drawOnto(withFace, pos, mask);
		}
	}
	void newMouth(const Image& img, Point pos, int num, const std::vector<MaskRect>& mask) override {
		withMouth = withFace;
		withMouthName = withFaceName + "_" + std::to_string(num);
		img.drawOnto(withMouth, pos, mask);
	}
	void write() override {
		if (!withMouthName.empty()) {
			write(withMouth, basePath/fs::u8path(withMouthName + ".png"));
		}
		else if (!withFaceName.empty()) {
			write(withFace, basePath/fs::u8path(withFaceName + ".png"));
		}
		else {
			write(base, basePath/fs::u8path(baseName + ".png"));
		}
	}
	virtual void write(Image& img, fs::path path) {
		img.writePNG(path);
	}
};

#if ENABLE_MULTITHREADED
class MTCompositedBupOutputter: public CompositedBupOutputter {
	ThreadedImageSaver writer;
public:
	void write(Image& img, fs::path path) override {
		writer.enqueue(std::move(img), std::move(path));
	}
};
#endif

std::unique_ptr<BupOutputter> BupOutputter::makeComposited() {
#if ENABLE_MULTITHREADED
	return std::make_unique<MTCompositedBupOutputter>();
#else
	return std::make_unique<CompositedBupOutputter>();
#endif
}
