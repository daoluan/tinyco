#include "string.h"

#include <string>
#include <sstream>
#include <vector>
#include <iterator>

namespace tinyco {
namespace string {

template <typename Out>
void Split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> Split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  Split(s, delim, std::back_inserter(elems));
  if (elems.empty()) elems.push_back(s);
  return elems;
}
}
}
