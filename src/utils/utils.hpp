#pragma once
#include "../http/http.hpp"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <string>

namespace utils {
size_t webStrToSizeT(const http::webStr &wstr);
bool matchEndpoint(const std::string &endpoint, const http::webStr &uri);
std::string getHttpStatusMessage(int statusCode);
int getPortNumber(int fd);
bool startsWith(std::string const &prefix, std::string const &str);
std::string getMimeType(const std::string &filename);
std::string intToString(int num);
} // namespace utils
