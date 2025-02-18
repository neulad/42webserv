#include "http.hpp"
#include "../server/FilefdFactory.hpp"
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

// Buffer
http::webbuf::webbuf(int size) {
  this->size = size;
  this->buffer = new char[size + 1];
  this->cursor = 0;
}
http::webbuf::~webbuf() { delete this->buffer; }

char *http::webbuf::getBuf() { return this->buffer; }
size_t http::webbuf::getCursor() { return this->cursor; }
void http::webbuf::setCursor(size_t newCursor) { this->cursor = newCursor; }
size_t http::webbuf::getSize() { return size; }

void http::webbuf::readBuf(int fd) {
  int nbytes = read(fd, this->buffer + cursor, this->size - cursor);
  if (nbytes < 0)
    throw std::runtime_error("error reading");
  cursor += nbytes;
  buffer[cursor] = '\0';
}
// /Buffer

// Request
http::Request::Request() {}
http::Request::~Request() {}

const char *http::Request::getHeader(char const *key) const {
  for (size_t i = 0; i < _headers.size(); ++i) {
    if (!strcmp(_headers[i].first, key))
      return _headers[i].second;
  }
  return NULL;
};
// /Request

// Connection
http::Connection::Connection(srvparams const &params)
    : curBuf(0), cursor(0), status(http::NOTHING_DONE), rnrnCounter(0) {
  buffers[0] = new webbuf(params.bufferSize);
  buffers[1] = new webbuf(params.bufferSize);
};

http::Connection::~Connection() {
  delete buffers[0];
  delete buffers[1];
}

void http::Connection::hndlIncStrm(int event_fd) {
  buffers[curBuf]->readBuf(event_fd);

  char *buffer = buffers[curBuf]->getBuf();
  char const *rnrnStr = "\r\n\r\n";
  char delim;
  char *bgnRnrn;
  while (buffer[cursor]) {
    if (buffer[cursor] == rnrnStr[rnrnCounter]) {
      ++rnrnCounter;
      if (rnrnCounter == 1) {
        bgnRnrn = buffer + cursor;
        ++cursor;
        continue;
      }
      if (rnrnCounter == 2) {
        *bgnRnrn = '\0';
        ++cursor;
        if (status == VALUE || status == HTTPVERS)
          curReq._headers.push_back(
              std::pair<char *, char *>(buffer + cursor, NULL));
        if (status == HTTPVERS || status == VALUE)
          status = KEY;
        continue;
      }
      if (rnrnCounter == 3) {
        ++cursor;
        continue;
      }
      if (rnrnCounter == 4) {
        if (status == KEY)
          curReq._headers.erase(curReq._headers.end());
        status = HEADERS_DONE;
        ++cursor;

        break;
      }
    } else
      rnrnCounter = 0;
    switch (status) {
    case NOTHING_DONE:
      delim = ' ';
      curReq.setMethod(buffer + cursor);
      break;
    case URI:
      delim = ' ';
      break;
    case HTTPVERS:
      delim = '\r';
      break;
    case KEY:
      delim = ':';
      break;
    case VALUE:
      delim = '\r';
      break;
    default:;
    }
    while (buffer[cursor] && buffer[cursor] != delim)
      ++cursor;
    if (buffer[cursor] != delim) {
      curBuf = (curBuf + 1) % 2;
      cursor = 0;
      return;
    }
    if (delim == ' ' || delim == ':') {
      buffer[cursor] = '\0';
      do
        ++cursor;
      while (buffer[cursor] == ' ');
    } else {
      ++rnrnCounter;
      bgnRnrn = buffer + cursor;
      ++cursor;
      continue;
    }
    switch (status) {
    case NOTHING_DONE:
      status = URI;
      curReq.setUri(buffer + cursor);
      break;
    case URI:
      status = HTTPVERS;
      curReq.setHttpvers(buffer + cursor);
      break;
    case KEY:
      status = VALUE;
      curReq._headers[curReq._headers.size() - 1].second = buffer + cursor;
      break;
    default:;
    }
    ++cursor;
  }
}
// /Connection

// Response
http::Response::Response(srvparams const &params)
    : protocol(params.protocol), statusCode(http::OK), statusMessage("OK") {}
http::Response::~Response() {
  for (std::map<std::string, void *>::iterator it = hooksMap.begin();
       it != hooksMap.end(); ++it) {
    if (it->second != NULL && deleters[it->first] != NULL) {
      deleters[it->first](it->second);
    }
  }
}
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
bool http::Response::isBodyReady() {
  return !body.empty() || !bodyPath.empty();
}
void http::Response::end(int event_fd, FilefdFactory &filefdfaq) {
  struct stat stat_buf;
  int filefd = -1;
  size_t fileSize = -1;
  response << protocol << " " << statusCode << " " << statusMessage << "\r\n";
  for (size_t i = 0; i < headers.size(); ++i) {
    response << headers[i].first << ": " << headers[i].second << "\r\n";
  }
  response << "\r\n";

  if (!bodyPath.empty()) {
    filefd = open(bodyPath.c_str(), O_RDONLY);
    if (filefd == -1)
      throw http::HttpError(strerror(errno), http::InternalServerError);
    fstat(filefd, &stat_buf);
    fileSize = stat_buf.st_size;
  } else
    response << body;

  std::string const &temp = response.str();
  send(event_fd, temp.c_str(), strlen(temp.c_str()), 0);
  if (filefd != -1) {
    int offset = sendfile(event_fd, filefd, NULL, filefdfaq.getChunkSize());
    if (offset < stat_buf.st_size)
      filefdfaq.addFdoffset(event_fd, filefd, fileSize, offset);
  }
}
// /Response
