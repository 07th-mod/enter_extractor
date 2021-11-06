#pragma once

#include <vector>
#include <string>
#include <cstring>
#include "FS.hpp"
#include "HeaderStructs.hpp"

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
	bool operator==(const Size& other) const {
		return !std::memcmp(this, &other, sizeof(other));
	}
	bool operator!=(const Size& other) const {
		return !(*this == other);
	}
};

struct Point {
	int x;
	int y;
	bool operator==(const Point& other) const {
		return !std::memcmp(this, &other, sizeof(other));
	}
	bool operator!=(const Point& other) const {
		return !(*this == other);
	}
};

struct Image {
	Size size;
	std::vector<Color> colorData;

	inline Image(Size size = {0, 0}, Color base = Color()): size(size), colorData(std::vector<Color>(size.area(), base)) {}

	inline const Color &pixel(int x, int y) const {
		return colorData[size.width * y + x];
	}

	inline Color& pixel(int x, int y) {
		return colorData[size.width * y + x];
	}

	void drawOnto(Image &image, Point point, Point sourcePoint, Size section) const;
	void drawOnto(Image &image, Point point, Size section) const;
	void drawOnto(Image &image, Point point, std::vector<MaskRect> sections) const;
	/// Resizes the image, copying the right and bottom pixels into the new sections
	Image resizeClampToEdge(Size newSize) const;

	Image resized(Size newSize) const;

	/// Resizes image without keeping contents intact
	inline void fastResize(Size newSize) {
		size = newSize;
		colorData.resize(size.area());
	}

	int writePNG(const fs::path &filename, const std::string &title = "") const;

	static Image readPNG(const fs::path &filename);

	bool empty() const { return colorData.empty(); }

	bool operator==(const Image& other) const;
};

enum class PNGColorType { GRAY, RGB, RGBA };
int writePNG(const fs::path &filename, PNGColorType color, Size size, const uint8_t *data, const std::string &title = "");
