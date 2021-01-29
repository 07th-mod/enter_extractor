#pragma once

#include <boost/filesystem.hpp>

// Defined in Pic.cpp
int processPic(std::ifstream &in, const boost::filesystem::path &output);
// Defined in Bup.cpp
int processBup(std::ifstream &in, const boost::filesystem::path &output);
// Defined in Txa.cpp
int processTxa(std::ifstream &in, const boost::filesystem::path &output);
// Defined in Msk.cpp
int processMsk3(std::ifstream &in, const boost::filesystem::path &output);
// Defined in Msk.cpp
int processMsk4(std::ifstream &in, const boost::filesystem::path &output);
