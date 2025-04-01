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
std::string getInterpreter(const std::string& path, const std::map<std::string, std::string>& extMap);
bool isCgi(const std::string &path, const std::map<std::string, std::string>& extMap);
std::vector<std::string> split(const std::string& str, char delimiter);