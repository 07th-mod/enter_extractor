#pragma once

#if USE_BOOST_FS
#include <boost/filesystem.hpp>
namespace fs {
	using namespace boost::filesystem;
	// Won't work on Windows, but the boost filesystem option is mostly for older macOS
	inline const std::string& u8path(const std::string& p) { return p; }
}
#else
#include <filesystem>
namespace fs {
	using namespace std::filesystem;
	typedef std::ifstream ifstream;
	typedef std::ofstream ofstream;
}
#endif

#ifdef _WIN32
# define FOPEN(file, mode) _wfopen(file, L##mode)
#else
# define FOPEN(file, mode) fopen(file, mode)
#endif
