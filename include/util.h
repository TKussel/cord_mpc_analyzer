/**
 \file            util.cpp
 \author          Tobias Kussel
 \email           kussel@cbs.tu-darmstadt.de
 \organization    Computational Biology and Simulation Group (CBS)
                  TU Darmstadt

 * TODO: Include MOTION Copyright
*/

#pragma once

#include "typedefs.h"
#include <algorithm>  //std::find_if
#include <cctype>     //std::isspace
#include <iterator>
#include <sstream>
#include <string>
#include <vector>


namespace CORDMPC {

constexpr size_t bit_to_bytes(size_t b) { return (b + 7) / 8; }

/**
 * Functions to trim whitespace from strings
 */
// trim from start (in place)
static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
          s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
  ltrim(s);
  return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
  rtrim(s);
  return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
  trim(s);
  return s;
}

/**
 *  Split delimiter separated strings into containers
 */
template <typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}
std::vector<std::string> split(const std::string &s, char delim);

// safeGetline to handle \n or \r\n from Stackoverflow User user763305
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf#6089413
std::istream &safeGetline(std::istream &, std::string &);

} // end namespace CORDMDR
