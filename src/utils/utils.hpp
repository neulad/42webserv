#pragma once
#include "../http/http.hpp"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <string>
#include <vector>

namespace utils {
size_t webStrToSizeT(const http::webStr &wstr);
bool cmpWebStrs(http::webStr str1, http::webStr str2);
bool cmpWebStrs(http::webStr str1, char *str2);
bool cmpWebStrs(char *str1, http::webStr str2);
bool matchEndpoint(const std::string &endpoint, const http::webStr &uri);
} // namespace utils
