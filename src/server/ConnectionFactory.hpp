#pragma once

#include "../http/http.hpp"
#include <cstddef>
#include <map>

class ConnectionFactory {
private:
  std::map<int, http::Connection *> connections;

public:
  bool ifExists(int fd);
  http::Connection &getConnection(int fd);
  void addConnection(http::Connection *conn, int fd);
  void delConnection(int fd);
  size_t getLen() const;
  ConnectionFactory();
  ~ConnectionFactory();
};
