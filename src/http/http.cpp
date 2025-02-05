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

// Request
http::Request::Request(srvparams const &params)
    : buffers_status(http::NOTHING_DONE), request_line_len(0),
      header_buffer(params.client_header_buffer_size) {}
http::Request::~Request() {}

void http::Request::handleData(int fd, srvparams const &params) {
  // TODO: handling parsing
  // TODO: body handling if it fits into header buffer
  if (!header_buffer.isFull()) {
    header_buffer.readBuf(fd);
    // TODO: we will have to check for this line in all of the buffers.
    if (utils::seqPresent((char *)"\r\n\r\n", header_buffer.getBuffer())) {
      header_buffer.setFull();
      buffers_status = HEADERS_DONE;
      // TODO: ADD \0 THAT it's the end of the buffer, the rest to the body
      // class
    }
    if (buffers_status == NOTHING_DONE) {
      char *nloc = strchr(header_buffer.getBuffer() + line_len, '\n');
      if (nloc != NULL) {
        buffers_status = REQUEST_LINE_DONE;
        line_len = nloc - header_buffer.getBuffer();
      } else {
        line_len = header_buffer.getEnd();
      }
      if (line_len > params.large_client_header_buffers_size)
        throw http::HttpError(http::URITooLong);
    }
  }
}
// /Request
