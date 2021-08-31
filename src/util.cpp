/**
 \file            util.cpp
 \author          Tobias Kussel
 \email           kussel@cbs.tu-darmstadt.de
 \organization    Computational Biology and Simulation Group (CBS)
                  TU Darmstadt

 * TODO: Include MOTION Copyright
*/

#include "../include/util.h"
#include "../include/typedefs.h"
#include "../include/config.h"

#include <cctype>
#include <functional>
#include <iomanip>
#include <iterator>


namespace CORDMPC {

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

// safeGetline to handle \n or \r\n from Stackoverflow User user763305
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf#6089413
std::istream& safeGetline(std::istream& is, std::string& t) {
  t.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream.
  // Code that uses streambuf this way must be guarded by a sentry object.
  // The sentry object performs various tasks,
  // such as thread synchronization and updating the stream state.

  std::istream::sentry se(is, true);
  std::streambuf* sb = is.rdbuf();

  for (;;) {
    int c = sb->sbumpc();
    switch (c) {
      case '\n':
        return is;
      case '\r':
        if (sb->sgetc() == '\n') {
          sb->sbumpc();
        }
        return is;
      case std::streambuf::traits_type::eof():
        // Also handle the case when the last line has no line ending
        if (t.empty()) {
          is.setstate(std::ios::eofbit);
        }
        return is;
      default:
        t += (char)c;
    }
  }
}

} // end namespace CORDMPC
