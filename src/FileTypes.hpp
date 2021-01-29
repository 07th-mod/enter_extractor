#pragma once

#include <iostream>
#include "FS.hpp"

// Defined in Pic.cpp
int processPic(std::istream &in, const fs::path &output);
// Defined in Bup.cpp
int processBup(std::istream &in, const fs::path &output);
// Defined in Txa.cpp
int processTxa(std::istream &in, const fs::path &output);
// Defined in Msk.cpp
int processMsk3(std::istream &in, const fs::path &output);
// Defined in Msk.cpp
int processMsk4(std::istream &in, const fs::path &output);
