#pragma once

// Use 0 to get better IDE autocomplete (but disable PS3 support)
#ifndef SHOULD_TEMPLATE
#  define SHOULD_TEMPLATE 1
#endif

#ifndef ENABLE_MULTITHREADED
#  define ENABLE_MULTITHREADED 0
#endif

#include <boost/filesystem.hpp>

extern bool SHOULD_WRITE_DEBUG_IMAGES;
extern boost::filesystem::path debugImagePath;
