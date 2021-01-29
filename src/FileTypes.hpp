#pragma once

#include <iostream>
#include <boost/filesystem.hpp>

// Defined in Pic.cpp
int processPic(std::istream &in, const boost::filesystem::path &output);
// Defined in Bup.cpp
int processBup(std::istream &in, const boost::filesystem::path &output);
// Defined in Txa.cpp
int processTxa(std::istream &in, const boost::filesystem::path &output);
// Defined in Msk.cpp
int processMsk3(std::istream &in, const boost::filesystem::path &output);
// Defined in Msk.cpp
int processMsk4(std::istream &in, const boost::filesystem::path &output);
