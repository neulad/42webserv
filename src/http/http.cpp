#include "http.hpp"
#include "../utils/utils.hpp"
#include <cstring>
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
int http::webbuf::getEnd() { return this->end; }
void http::webbuf::setEnd(int end) {
  this->buffer[end] = '\0';
  this->end = end;
  setFull();
}
int http::webbuf::getSize() { return size; }
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

http::Request::Request(srvparams const &params)
    : header_buffer(params.client_header_buffer_size) {}
http::Request::~Request() {}

void http::Request::handleData(int fd, srvparams const &params) {
  if (!header_buffer.isFull()) {
    header_buffer.readBuf(fd);
    if (utils::seqPresent((char *)"\r\n\r\n", header_buffer.getBuffer())) {
      header_buffer.setFull();
    } else if (!utils::anyPresent((char *)"\n\r", header_buffer.getBuffer() +
                                                      header_buffer.getSize() -
                                                      1))
      throw http::HttpError("");
  } else {
  }
}