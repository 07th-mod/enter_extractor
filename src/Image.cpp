#include "Image.hpp"
extern "C" {
#include <png.h>
}
#include <cstdio>
#define throwing_assert(x) if (!(x)) { throw std::runtime_error(std::string("Failed assertion ") + #x); }

void Image::drawOnto(Image &image, Point point, Point sourcePoint, Size section) const {
	throwing_assert(sourcePoint.x + section.width <= size.width && sourcePoint.y + section.height <= size.height);
	throwing_assert(point.x + section.width <= image.size.width && point.y + section.height <= image.size.height);

	for (int y = 0; y < section.height; y++) {
		for (int x = 0; x < section.width; x++) {
			image.pixel(x + point.x, y + point.y) = this->pixel(x + sourcePoint.x, y + sourcePoint.y);
		}
	}
}

void Image::drawOnto(Image &image, Point point, Size section) const {
	drawOnto(image, point, {0, 0}, section);
}

void Image::drawOnto(Image &image, Point point, std::vector<MaskRect> sections) const {
	for (const auto& section : sections) {
		throwing_assert(section.x2 <= size.width && section.y2 <= size.height);
		throwing_assert(point.x + section.x2 <= image.size.width && point.y + section.y2 <= image.size.height);
		for (int y = section.y1; y < section.y2; y++) {
			for (int x = section.x1; x < section.x2; x++) {
				image.pixel(x + point.x, y + point.y) = this->pixel(x, y);
			}
		}
	}
}

Image Image::resizeClampToEdge(Size newSize) const {
	throwing_assert(size.width > 0 && size.height > 0);
	Image out(newSize);
	int minW = std::min(size.width, newSize.width);
	int minH = std::min(size.height, newSize.height);
	for (int y = 0; y < minH; y++) {
		memcpy(&out.pixel(0, y), &pixel(0, y), minW * sizeof(Color));
		for (int x = minW; x < newSize.width; x++) {
			out.pixel(x, y) = pixel(size.width - 1, y);
		}
	}
	for (int y = minH; y < newSize.height; y++) {
		memcpy(&out.pixel(0, y), &out.pixel(0, size.height - 1), newSize.width * sizeof(Color));
	}
	return out;
}

Image Image::resized(Size newSize) const {
	Image newImage(newSize);
	this->drawOnto(newImage, {0, 0}, {std::min(size.width, newSize.width), std::min(size.height, newSize.height)});
	return newImage;
}

bool Image::operator==(const Image& other) const {
	if (size != other.size) { return false; }
	return !std::memcmp(colorData.data(), other.colorData.data(), colorData.size() * sizeof(Color));
}

int Image::writePNG(const fs::path &filename, const std::string &title) const {
	return ::writePNG(filename, PNGColorType::RGBA, size, reinterpret_cast<const uint8_t *>(colorData.data()), title);
}

// Based off http://www.labbookpages.co.uk/software/imgProc/libPNG.html
int writePNG(const fs::path &filename, PNGColorType color, Size size, const uint8_t *data, const std::string &title) {
	int code = 0;
	png_structp pngPtr = NULL;
	png_infop infoPtr = NULL;
	png_bytep row = NULL;
	png_int_32 pcolor = 0;
	int pitch = 0;

	switch (color) {
		case PNGColorType::GRAY: pcolor = PNG_COLOR_TYPE_GRAY; pitch = 1; break;
		case PNGColorType::RGB:  pcolor = PNG_COLOR_TYPE_RGB;  pitch = 3; break;
		case PNGColorType::RGBA: pcolor = PNG_COLOR_TYPE_RGBA; pitch = 4; break;
	}

	FILE *file = FOPEN(filename.c_str(), "wb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file %s for writing\n", filename.string().c_str());
		code = 1;
		goto finalize;
	}

	pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (pngPtr == NULL) {
		fprintf(stderr, "Could not allocate PNG write struct\n");
		code = 1;
		goto finalize;
	}

	infoPtr = png_create_info_struct(pngPtr);
	if (infoPtr == NULL) {
		fprintf(stderr, "Could not allocate PNG info struct\n");
		code = 1;
		goto finalize;
	}

	if (setjmp(png_jmpbuf(pngPtr))) {
		fprintf(stderr, "Error during PNG creation\n");
		code = 1;
		goto finalize;
	}

	png_init_io(pngPtr, file);

	// Write header (8 bit color depth)
	png_set_IHDR(pngPtr, infoPtr, size.width, size.height, /*bit depth*/ 8, pcolor, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	if (title.size() > 0) {
		png_text titleText;
		titleText.compression = PNG_TEXT_COMPRESSION_NONE;
		titleText.key = (char *)"Title";
		titleText.text = (char *)title.c_str();
		png_set_text(pngPtr, infoPtr, &titleText, 1);
	}

	png_write_info(pngPtr, infoPtr);

//	row = (png_bytep)malloc(4 * this->size.width * sizeof(png_byte));

	for (int y = 0; y < size.height; y++) {
//		for (int x = 0; x < this->size.width; x++) {
//			*(Color *)&row[x * 4] = this->pixel(x, y);
//		}
		png_write_row(pngPtr, &data[y * size.width * pitch]);
	}

	png_write_end(pngPtr, NULL);

finalize:
	if (file != NULL) { fclose(file); }
	if (infoPtr != NULL) { png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1); }
	if (pngPtr != NULL) { png_destroy_write_struct(&pngPtr, (png_infopp)NULL); }
	if (row != NULL) { free(row); }
	return code;
}

Image Image::readPNG(const fs::path &filename) {
	Image out;
	bool success = false;
	png_structp pngPtr = NULL;
	png_infop infoPtr = NULL;

	FILE *file = FOPEN(filename.c_str(), "rb");
	if (!file) { goto finalize; }

	pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr) { goto finalize; }

	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) { goto finalize; }

	if (setjmp(png_jmpbuf(pngPtr))) { goto finalize; }

	png_init_io(pngPtr, file);
	png_read_info(pngPtr, infoPtr);

	png_uint_32 width, height;
	int bit_depth, color_type;
	png_get_IHDR(pngPtr, infoPtr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	png_set_strip_16(pngPtr);
	png_set_expand(pngPtr);
	png_set_gray_to_rgb(pngPtr);
	png_set_add_alpha(pngPtr, 0xFF, PNG_FILLER_AFTER);
	png_read_update_info(pngPtr, infoPtr);

	out.fastResize({static_cast<int>(width), static_cast<int>(height)});
	for (int y = 0; y < out.size.height; y++) {
		png_read_row(pngPtr, (png_bytep)&out.pixel(0, y), NULL);
	}

	success = true;

finalize:
	if (file) { fclose(file); }
	if (infoPtr) { png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1); }
	if (pngPtr) { png_destroy_read_struct(&pngPtr, NULL, NULL); }
	if (success) {
		return out;
	} else {
		throw std::runtime_error("Failed to read PNG from " + filename.string());
	}
}
