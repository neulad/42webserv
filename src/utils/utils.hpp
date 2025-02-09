#pragma once
#include <cstdarg>
#include <cstring>
#include <stdarg.h>
#include <string>
#include <vector>

namespace utils {
std::vector<std::string> splitBeforeNewline(std::string const &str,
                                            size_t const cursor);
bool iCompare(const std::string &str1, const std::string &str2);
// Checks if the sequence is present
bool seqPresent(char const *seq, ...);
bool anyPresent(char const *any, char *str);
bool matchEndpoint(const std::string &endpoint, const std::string &uri);
} // namespace utils
