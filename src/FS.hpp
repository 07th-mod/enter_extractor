#if USE_BOOST_FS
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#ifdef _WIN32
# define FOPEN(file, mode) _wfopen(file, L##mode)
#else
# define FOPEN(file, mode) fopen(file, mode)
#endif
