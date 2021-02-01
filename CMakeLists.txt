cmake_minimum_required (VERSION 3.5...3.17)

project(EnterExtractor LANGUAGES CXX)

option(BOOST_FILESYSTEM "Use boost filesystem instead of C++17 filesystem" OFF)
option(ENABLE_MULTITHREADING "Enable multithreaded PNG output for BUP conversion" ON)

set(CMAKE_FIND_FRAMEWORK LAST)

set(SOURCES src/Image.cpp src/Decompression.cpp src/HeaderStructs.cpp src/RegionChecker.cpp src/Pic.cpp src/CompositedBupOutputter.cpp src/PartsBupOutputter.cpp src/Bup.cpp src/Txa.cpp src/Msk.cpp src/main.cpp)
set(HEADERS src/Config.hpp src/Image.hpp src/Decompression.hpp src/HeaderStructs.hpp src/RegionChecker.hpp src/FileTypes.hpp src/FS.hpp src/BupOutputters.hpp)

add_executable(EnterExtractor ${SOURCES} ${HEADERS})

find_package(PNG REQUIRED)
target_link_libraries(EnterExtractor PRIVATE PNG::PNG)

set(BOOST_COMPONENTS locale)

if(${BOOST_FILESYSTEM})
	set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem)
	target_compile_definitions(EnterExtractor PRIVATE USE_BOOST_FS)
	target_compile_features(EnterExtractor PRIVATE cxx_std_14)
else()
	target_compile_features(EnterExtractor PRIVATE cxx_std_17)
endif()

if(${ENABLE_MULTITHREADING})
	find_package(Threads REQUIRED)
	target_link_libraries(EnterExtractor PRIVATE Threads::Threads)
	set(BOOST_COMPONENTS ${BOOST_COMPONENTS} thread)
	target_compile_definitions(EnterExtractor PRIVATE ENABLE_MULTITHREADED=1 BOOST_THREAD_VERSION=5)
else()
	target_compile_definitions(EnterExtractor PRIVATE ENABLE_MULTITHREADED=0)
endif()
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
foreach(component IN ITEMS ${BOOST_COMPONENTS})
	target_link_libraries(EnterExtractor PRIVATE Boost::${component})
endforeach()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
