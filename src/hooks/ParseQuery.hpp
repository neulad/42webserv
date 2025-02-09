#pragma once

#include "../http/http.hpp"
#include <map>

void parseQueryString(http::Request const &req, http::Response &res);
typedef std::map<std::string, std::string> queryStringType;
