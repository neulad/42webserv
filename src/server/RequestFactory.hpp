#pragma once

#include "server.hpp"
#include <map>

class RequestFactory {
private:
  std::map<int, server::Request> requests;

public:
  bool ifExists(int fd);
  server::Request &getRequest(int fd);
  void setRequest(server::Request const &req, int fd);
  void deleteRequest(int fd);
  RequestFactory();
  ~RequestFactory();
};
