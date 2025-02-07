#include "http.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

// Buffer
http::webbuf::webbuf(int size) {
  this->size = size;
  this->buffer = new char[size + 1];
  this->full = false;
  this->end = 0;
}
http::webbuf::~webbuf() { delete this->buffer; }

char *http::webbuf::getBuffer() { return this->buffer; }
size_t http::webbuf::getEnd() { return this->end; }
void http::webbuf::setEnd(int end) {
  this->buffer[end] = '\0';
  this->end = end;
  setFull();
}
size_t http::webbuf::getSize() { return size; }
bool http::webbuf::isFull() { return full; }
void http::webbuf::setFull() { full = true; }
void http::webbuf::readBuf(int fd) {
  if (full)
    return;
  int nbytes = read(fd, this->buffer + end, this->size - end);
  if (nbytes < 0)
    throw std::runtime_error("error reading");
  end += nbytes;
  buffer[end] = '\0';
  if (end == size)
    full = true;
}
// /Buffer

// Request
http::Request::Request(srvparams const &params)
    : headerBuffer(webbuf(params.headerBufferSize)),
      status(http::NOTHING_DONE) {}
http::Request::~Request() {}

void http::Request::handleData(int fd) { headerBuffer.readBuf(fd); }
// /Request

// Response
http::Response::Response(srvparams const &params)
    : protocol(params.protocol), statusCode(http::OK), statusMessage("OK") {}
http::Response::~Response() {}
void http::Response::setStatusCode(int statusCode) {
  statusCode = int(statusCode);
};
void http::Response::setStatusMessage(std::string const statusMessage_) {
  statusMessage = statusMessage_;
}
void http::Response::setHeader(std::string key, std::string value) {
  headers.push_back(std::pair<std::string, std::string>(key, value));
}
void http::Response::setBody(std::string const &body_) { body = body_; }
void http::Response::setBodyPath(std::string const bodyPath_) {
  bodyPath = bodyPath_;
}
void http::Response::end(int fd) {
  response << protocol << " " << statusCode << " " << statusCode << "\r\n";
  for (size_t i = 0; i < headers.size(); ++i) {
    response << headers[i].first << ": " << headers[i].second << "\r\n";
  }
  response << "\r\n";
  if (!bodyPath.empty())
    return;
  response << body;
  std::string const &temp = response.str();
  send(fd, temp.c_str(), strlen(temp.c_str()), 0);
}
// /Response
