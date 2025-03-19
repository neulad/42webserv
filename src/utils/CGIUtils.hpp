#pragma once

#include "../http/http.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

int safeOpen(std::string const &path, int mode);
pid_t safeFork();
std::string getQueryString(std::string uri);
std::string getScriptPath(std::string uri);
std::string readFileToString(const std::string &filename);
void setResHeader(std::string response, http::Response &res);
void setResBody(std::string response, http::Response &res);
bool isCgi(const std::string &path);
