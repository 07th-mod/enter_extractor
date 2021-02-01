#pragma once

#ifndef ENABLE_MULTITHREADED
#  define ENABLE_MULTITHREADED 0
#endif

#include "FS.hpp"

extern const char* currentFileName;
extern bool SHOULD_WRITE_DEBUG_IMAGES;
extern bool SAVE_BUP_AS_PARTS;
extern fs::path debugImagePath;
