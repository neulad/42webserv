#include "http.hpp"
#include <cstddef>
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
    : methodParsed(false), uriParsed(false), httpversParsed(false), currbuff(0),
      cursor(0), endseq_cnt(0), status(http::NOTHING_DONE), line_len(0) {
  header_buffers.push_back(webbuf(params.client_header_buffer_size));
}
http::Request::~Request() {}

void http::Request::handleData(int fd, srvparams const &params) {
  // if status = headers_done
  // parse and buffer the body
  if (!header_buffers[0].isFull()) {
    header_buffers[0].readBuf(fd);
    // TODO: we will have to check for this line in all of the buffers.
  } else {
    /**
     * Check if the large buffers for headers are created.
     * If not, initialize them
     */
    if (header_buffers.size() == 1) {
      for (int i = 0; i < params.large_client_header_buffers_number; ++i) {
        header_buffers.push_back(
            webbuf(params.large_client_header_buffers_size));
      }
      ++currbuff;
    }
    header_buffers[currbuff].readBuf(fd);
  }
  // Parsing
  while (cursor < header_buffers[currbuff].getEnd()) {
    char *headers_endseq = (char *)"\r\n\r\n";
    if (header_buffers[currbuff].getBuffer()[cursor] ==
        headers_endseq[endseq_cnt])
      ++endseq_cnt;
    ++cursor;
  }
  // /Parsing

  /**
   * Check for buffers, if the current is full
   * then go to the next one. If all of the buffers are full,
   * and the headers processing is not over yet - BadRequest 400
   */
  if (header_buffers[currbuff].isFull()) {
    ++currbuff;
    cursor = 0;
    if (currbuff == (size_t)params.large_client_header_buffers_number &&
        status != HEADERS_DONE)
      throw http::HttpError("Request line and headers are too long",
                            http::BadRequest);
  }
}
// /Request
