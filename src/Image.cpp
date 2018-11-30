#include "Image.hpp"
extern "C" {
#include <png.h>
}
#include <cstdio>
#define throwing_assert(x) if (!(x)) { throw std::runtime_error(std::string("Failed assertion ") + #x); }

void Image::drawOnto(Image &image, Point point, Size section) const {
	throwing_assert(section.width <= size.width && section.height <= size.height);
	throwing_assert(point.x + section.width <= image.size.width && point.y + section.height <= image.size.height);

	for (int y = 0; y < section.height; y++) {
		for (int x = 0; x < section.width; x++) {
			image.pixel(x + point.x, y + point.y) = this->pixel(x, y);
		}
	}
}

void Image::drawOntoCombine(Image &image, Point point, Size section, CombineMode mode) const {
	throwing_assert(section.width <= size.width && section.height <= size.height);
	throwing_assert(point.x + section.width <= image.size.width && point.y + section.height <= image.size.height);

	for (int y = 0; y < section.height; y++) {
		for (int x = 0; x < section.width; x++) {
			int targetX = x + point.x;
			int targetY = y + point.y;
			Color &dest = image.pixel(targetX, targetY);
			const Color &source = this->pixel(x, y);
			switch (mode) {
				case Image::CombineMode::SkipBlack:
					if (source.r != 0 || source.g != 0 || source.b != 0) {
						dest = source;
					}
					break;
				case Image::CombineMode::DestAlpha:
					dest.r = source.r;
					dest.g = source.g;
					dest.b = source.b;
				default:
					throw std::runtime_error("Used unsupported combine mode!");
			}
		}
	}
}

Image Image::resized(Size newSize) const {
	Image newImage(newSize);
	this->drawOnto(newImage, {0, 0}, {std::min(size.width, newSize.width), std::min(size.height, newSize.height)});
	return newImage;
}

// Based off http://www.labbookpages.co.uk/software/imgProc/libPNG.html
int Image::writePNG(const boost::filesystem::path &filename, const std::string &title) const {
	int code = 0;
	png_structp pngPtr = NULL;
	png_infop infoPtr = NULL;
	png_bytep row = NULL;

	FILE *file = fopen(filename.c_str(), "wb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file %s for writing\n", filename.c_str());
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
	png_set_IHDR(pngPtr, infoPtr, this->size.width, this->size.height, /*bit depth*/ 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	if (title.size() > 0) {
		png_text titleText;
		titleText.compression = PNG_TEXT_COMPRESSION_NONE;
		titleText.key = (char *)"Title";
		titleText.text = (char *)title.c_str();
		png_set_text(pngPtr, infoPtr, &titleText, 1);
	}

	png_write_info(pngPtr, infoPtr);

	//	row = (png_bytep)malloc(4 * image.size.width * sizeof(png_byte));

	for (int y = 0; y < this->size.height; y++) {
		//		for (int x = 0; x < image.size.width; x++) {
		//			*(Color *)&row[x * 4] = image.pixel(x, y);
		//		}
		png_write_row(pngPtr, (png_bytep)&this->pixel(0, y));
	}

	png_write_end(pngPtr, NULL);

finalize:
	if (file != NULL) { fclose(file); }
	if (infoPtr != NULL) { png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1); }
	if (pngPtr != NULL) { png_destroy_write_struct(&pngPtr, (png_infopp)NULL); }
	if (row != NULL) { free(row); }
	return code;
}
