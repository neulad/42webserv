#pragma once

#include "../http/http.hpp"
#include "../utils/utils.hpp"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <stdexcept>


void handleCgi(http::Request const &req, http::Response &res);