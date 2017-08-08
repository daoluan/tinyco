#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_

#include <vector>
#include <string>

namespace tinyco {
namespace string {

std::vector<std::string> Split(const std::string &s, char delim);
}
}
#endif
