#include "HandleJson.hpp"
#include <cstring>
#include <iostream>

void handleJson(http::Request const &req, http::Response &res) {
  (void)res;
  if (!strcmp(req.getMethod(), "GET") || !strcmp(req.getMethod(), "DELETE"))
    return;
  char const *contentType = req.getHeader("Content-Type");
  if (!strcmp(contentType, "application/json"))
    std::cout << "it's truly json!" << std::endl;
}
