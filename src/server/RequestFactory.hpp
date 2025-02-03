#pragma once

#include "../http/http.hpp"
#include <map>

class RequestFactory {
private:
  std::map<int, http::Request *> requests;

public:
  bool ifExists(int fd);
  http::Request &getRequest(int fd);
  void setRequest(http::Request *req, int fd);
  void deleteRequest(int fd);
  RequestFactory();
  ~RequestFactory();
};
