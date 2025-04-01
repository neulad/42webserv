#pragma once

#include "../http/http.hpp"
#include "../utils/utils.hpp"
#include "../utils/CGIUtils.hpp"
#include "../utils/CGIProcessHandling.hpp"

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
#include <cstddef>
#include <vector>


class CGI {
	private:
		std::string _extension;
		std::string _interpreter;
		void initMap();
	public:
		static std::map<std::string, std::string> _interpretersMap;
		CGI(const std::string &extension, const std::string &interpreter);
		static void handleCgi(http::Request const &req, http::Response &res);
};
