#pragma once

#include "../server/server.hpp"
#include <cstring>
#include <exception>
#include <string>

namespace http {
class HttpError : public std::exception {
private:
  char message[256]; // Fixed-size buffer to store the error message

public:
  HttpError(const char *msg) {
    std::strncpy(message, msg, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
  }
  const char *what() const throw() { return message; }
};
class webbuf {
private:
  char *buffer;
  int size;
  int end;
  bool full;

public:
  void setFull();
  bool isFull();
  void readBuf(int fd);
  int getSize();
  int getEnd();
  void setEnd(int end);
  char *getBuffer();
  webbuf(int size);
  ~webbuf();
};
class Request {
private:
  std::string method;
  std::string uri;
  std::string http_version;

public:
  void handleData(int fd, srvparams const &params);
  webbuf header_buffer;
  Request(srvparams const &params);
  Request &operator=(Request const &req);
  // TODO: Delete buffers
  ~Request();
};
}; // namespace http