#include "BupOutputters.hpp"

#include <fstream>
#include <iostream>

class PartsBupOutputter: public BupOutputter {
	fs::path basePath;
	fs::path partsPath;
	std::string baseName;
	std::string withFaceName;
	std::string withMouthName;
	Point facePos = {0, 0};
	Point mouthPos = {0, 0};
	Image base;
	Image withFace;
	Image tmp;

	enum class Blend {
		/// Images can be composited using normal alpha blending
		AlphaBlend,
		/// Images need a blend function that copies pixels if the src alpha is != 0
		CustomBlend,
		/// No blend function will properly recreate the image :(
		NotPossible,
	};

	Blend faceBlend, mouthBlend;

	struct ImageEntry {
		std::vector<std::pair<Point, std::string>> parts;
		Blend color = Blend::AlphaBlend;
	};

	std::vector<std::pair<std::string, std::vector<ImageEntry>>> expressions;

public:
	void setBase(fs::path basePath, Image img, std::string name) override {
		partsPath = basePath/name;
		fs::create_directory(partsPath);
		img.writePNG(partsPath/(name + ".png"));
		this->basePath = std::move(basePath);
		baseName = std::move(name);
		base = std::move(img);
	}

	Blend prepareImageInTmp(const Image& img, Point pos, const std::vector<MaskRect>& mask, const Image& target) {
		tmp.fastResize(img.size);
		std::fill(tmp.colorData.begin(), tmp.colorData.end(), Color());
		bool hasPartiallyTransparent = false;
		bool hasFullyTransparent = false;
		img.drawOnto(tmp, {0, 0}, mask);
		for (const auto& section : mask) {
			for (int y = section.y1; y < section.y2; y++) {
				for (int x = section.x1; x < section.x2; x++) {
					Color px = img.pixel(x, y);
					if (target.pixel(pos.x + x, pos.y + y).a == 0) {
						continue;
					}
					if (px.a == 0) {
						hasFullyTransparent = true;
					}
					else if (px.a < 255) {
						hasPartiallyTransparent = true;
					}
				}
			}
		}
		return hasFullyTransparent ? Blend::NotPossible
		     : hasPartiallyTransparent ? Blend::CustomBlend
		     : Blend::AlphaBlend;
	}

	void newFace(const Image& img, Point pos, const std::string &name, const std::vector<MaskRect>& mask) override {
		withFace = base;
		withMouthName = "";
		mouthBlend = Blend::AlphaBlend;
		facePos = pos;
		if (img.empty()) {
			withFaceName = "";
			faceBlend = Blend::AlphaBlend;
		}
		else {
			img.drawOnto(withFace, pos, mask);
			withFaceName = baseName + "_" + name;
			faceBlend = prepareImageInTmp(img, pos, mask, base);
			tmp.writePNG(partsPath/(withFaceName + ".png"));
		}
		expressions.push_back({name, {}});
	}

	void newMouth(const Image& img, Point pos, int num, const std::vector<MaskRect>& mask) override {
		withMouthName = (withFaceName.empty() ? baseName : withFaceName) + "_" + std::to_string(num);
		mouthPos = pos;
		mouthBlend = prepareImageInTmp(img, pos, mask, withFace);
		tmp.writePNG(partsPath/(withMouthName + ".png"));
	}

	void write() override {
		expressions.back().second.emplace_back();
		ImageEntry& cur = expressions.back().second.back();

		auto addColor = [&](Blend type){
			if (cur.color == Blend::AlphaBlend) {
				cur.color = type;
			} else if (type == Blend::NotPossible) {
				cur.color = type;
			}
		};
		auto path = [&](const std::string& name){
			return baseName + "/" + name + ".png";
		};

		cur.parts.push_back({{0, 0}, path(baseName)});
		if (!withFaceName.empty()) {
			cur.parts.push_back({facePos, path(withFaceName)});
			addColor(faceBlend);
		}
		if (!withMouthName.empty()) {
			cur.parts.push_back({mouthPos, path(withMouthName)});
			addColor(mouthBlend);
		}
		if (cur.color == Blend::NotPossible) {
			std::cerr << "Warning: " << withFaceName << " has fully transparent overridden pixels, it may not be properly renderable" << std::endl;
		}
	}

	void writeJSON() {
		fs::path name = basePath/(baseName + ".json");
		// When you didn't bother to import a json library
		// Hopefully no one puts weird characters in their expression names
#ifdef USE_BOOST_FS
		fs::ofstream out(name, std::ios::binary);
#else
		std::ofstream out(name, std::ios::binary);
#endif

		auto lastComma = [&](const auto& item, const auto& arr){
			if (&item != &arr.back()) {
				out << ",";
			}
			out << std::endl;
		};

		out << "{" << std::endl;
		for (const auto& exp : expressions) {
			out << "\t\"" << exp.first << "\": [" << std::endl;
			for (const auto& img : exp.second) {
				out << "\t\t{" << std::endl;
				out << "\t\t\t\"blend\": \"";
				switch (img.color) {
#define OUT(x) case Blend::x: out << #x; break;
						OUT(AlphaBlend)
						OUT(CustomBlend)
						OUT(NotPossible)
#undef OUT
				}
				out << "\"," << std::endl;
				out << "\t\t\t\"parts\": [" << std::endl;
				for (const auto& part : img.parts) {
					out << "\t\t\t\t{\"path\": \"";
					out << part.second;
					out << "\", \"x\": ";
					out << part.first.x;
					out << ", \"y\": ";
					out << part.first.y;
					out << "}";
					lastComma(part, img.parts);
				}
				out << "\t\t\t]" << std::endl;
				out << "\t\t}";
				lastComma(img, exp.second);
			}
			out << "\t]";
			lastComma(exp, expressions);
		}
		out << "}" << std::endl;
	}

	~PartsBupOutputter() {
		writeJSON();
	}
};

std::unique_ptr<BupOutputter> BupOutputter::makeParts() {
	return std::make_unique<PartsBupOutputter>();
}
