#include "http.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

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

void http::Request::parseRequestLine(char **buffer) {
  method = *buffer;
  while (**buffer != ' ') {
    ++*buffer;
  }
  **buffer = '\0';
  uri = *buffer + 1;
  while (**buffer != ' ') {
    ++*buffer;
  }
  **buffer = '\0';
  httpvers = *buffer + 1;
  while (**buffer != '\n') {
    ++*buffer;
  }
  **buffer = '\0';
  ++*buffer;
}
#include <stdio.h>
void http::Request::handleData(int fd) {
  headerBuffer.readBuf(fd);

  //
  char* buffer = headerBuffer.getBuffer();
  parseRequestLine(&buffer);
  while (status < http::HEADERS_DONE) {
    // parse header
    char* key = buffer;
    while (*buffer != ':') {
      ++buffer;
    }
    *buffer = '\0';
    buffer += 2;

    char* value = buffer;
    while (*buffer != '\r' && buffer) {
      ++buffer;
    }
    *buffer = '\0';
    buffer += 2;
    if (*buffer == '\r') {
      buffer += 2;
      status = http::HEADERS_DONE;
    }
    headers.push_back(std::make_pair(key, value));
  }
  // print headers
  std::cout << "\n\n@@@Method: " << method << std::endl;
  std::cout << "@@@URI: " << uri << std::endl;
  std::cout << "@@@HTTP Version: " << httpvers << std::endl;
  for (size_t i = 0; i < headers.size(); ++i) {
    std::cout << headers[i].first << ":: " << headers[i].second << std::endl;
  }
}
// /Request
