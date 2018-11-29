cmake_minimum_required (VERSION 2.8.11)

project(EnterExtractor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

option(ENABLE_MULTITHREADING "Enable multithreaded PNG output for BUP conversion" ON)

set(CMAKE_FIND_FRAMEWORK LAST)

set(SOURCES src/Image.cpp src/Decompression.cpp src/RegionChecker.cpp src/main.cpp)
set(HEADERS src/Config.hpp src/Image.hpp src/Decompression.hpp src/HeaderStructs.hpp src/RegionChecker.hpp src/Pic.hpp src/Bup.hpp)

add_executable(EnterExtractor ${SOURCES} ${HEADERS})

find_package(PNG REQUIRED)
include_directories(EnterExtractor ${PNG_INCLUDE_DIR})
target_link_libraries(EnterExtractor ${PNG_LIBRARY})

#set(Boost_USE_STATIC_LIBS ON)
if(${ENABLE_MULTITHREADING})
	find_package(Threads REQUIRED)
	target_link_libraries(EnterExtractor ${CMAKE_THREAD_LIBS_INIT})

	find_package(Boost REQUIRED COMPONENTS filesystem locale thread)

	add_definitions(-DENABLE_MULTITHREADED=1)
else()
	find_package(Boost REQUIRED COMPONENTS filesystem locale)

	add_definitions(-DENABLE_MULTITHREADED=0)
endif()
include_directories(EnterExtractor ${Boost_INCLUDE_DIRS})
target_link_libraries(EnterExtractor ${Boost_LIBRARIES})

#find_package(Iconv)
#if(Iconv_FOUND)
#	target_link_libraries(EnterExtractor ${Iconv_LIBRARIES})
#endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
