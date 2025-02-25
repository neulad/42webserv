#pragma once

#include "../http/http.hpp"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

void handleCgi(http::Request const &req, http::Response &res);
