#pragma once

#include "../http/http.hpp"

class StaticHandler {
  std::string dirPath;

public:
  StaticHandler(const std::string &path) : dirPath(path) {}
  void operator()(http::Request const &req, http::Response &res) const;
};
