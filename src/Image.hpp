#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <boost/filesystem.hpp>

struct Color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	Color() = default;
	inline Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {}

	inline bool operator==(const Color &other) const {
		return !std::memcmp(this, &other, sizeof(other));
	}
};

struct Size {
	int width;
	int height;
	int area() const {
		return width * height;
	}
};

struct Point {
	int x;
	int y;
};

struct Image {
	enum class CombineMode {
		SkipBlack,
		DestAlpha,
	};

	Size size;
	std::vector<Color> colorData;

	inline Image(Size size, Color base = Color()): size(size), colorData(std::vector<Color>(size.area(), base)) {}

	inline const Color &pixel(int x, int y) const {
		return colorData[size.width * y + x];
	}

	inline Color& pixel(int x, int y) {
		return colorData[size.width * y + x];
	}

	void drawOnto(Image &image, Point point, Size section) const;
	void drawOntoCombine(Image &image, Point point, Size section, CombineMode mode) const;

	Image resized(Size newSize) const;

	/// Resizes image without keeping contents intact
	inline void fastResize(Size newSize) {
		size = newSize;
		colorData.resize(size.area());
	}

	int writePNG(const boost::filesystem::path &filename, const std::string &title = "") const;
};

