#pragma once

// Use 0 to get better IDE autocomplete (but disable PS3 support)
#ifndef SHOULD_TEMPLATE
#  define SHOULD_TEMPLATE 1
#endif

// Whether or not to write debug images to /tmp/chunks
#ifndef SHOULD_WRITE_DEBUG_IMAGES
#  define SHOULD_WRITE_DEBUG_IMAGES 0
#endif

#ifndef ENABLE_MULTITHREADED
#  define ENABLE_MULTITHREADED 0
#endif

#include <boost/filesystem.hpp>

const boost::filesystem::path debugImagePath = "/tmp/chunks";
