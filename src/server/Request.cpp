#include "../utils/utils.hpp"
#include "server.hpp"
#include <sstream>
#include <stdexcept>
#include <unistd.h>

server::Request::Request() {
  methodReady = false;
  urlReady = false;
  protocolReady = false;
  headersReady = false;
  bodyReady = false;
}
server::Request::~Request() {}
server::Request &server::Request::operator=(server::Request const &req) {
  methodReady = req.methodReady;
  urlReady = req.urlReady;
  protocolReady = req.protocolReady;
  headersReady = req.headersReady;
  bodyReady = req.bodyReady;
  method = req.method;
  url = req.url;
  protocol = req.protocol;
  headers = req.headers;
  // TODO copy the body
  return *this;
}

void server::Request::handleData(int fd, char *buffer, int buffer_size) {
  int bytes_read = read(fd, buffer, buffer_size);
  if (bytes_read <= 0)
    throw std::runtime_error("Error reading TCP");
  buffer[bytes_read] = '\0';
  std::istringstream iss(buffer);
}