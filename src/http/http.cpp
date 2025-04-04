#include "http.hpp"
#include "../server/FilefdFactory.hpp"
#include "../utils/utils.hpp"
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

// webbuf
http::webbuf::webbuf(int size) {
  this->size = size;
  this->buffer = new char[size + 1];
  this->cursor = 0;
}
http::webbuf::~webbuf() { delete[] this->buffer; }

char *http::webbuf::getBuf() { return this->buffer; }
size_t http::webbuf::getCursor() { return this->cursor; }
void http::webbuf::setCursor(size_t newCursor) { this->cursor = newCursor; }
size_t http::webbuf::getSize() { return size; }

void http::webbuf::readBuf(int fd) {
  if (this->cursor == this->size)
    cursor = 0;
  int nbytes = read(fd, this->buffer + cursor, this->size - cursor);
  if (nbytes < 0)
    throw std::runtime_error("error reading");
  cursor += nbytes;
  buffer[cursor] = '\0';
}
// /webbuf

// webStr
long http::webStr::size() const {
  if (pos && !_posSize) {
    while (pos[_size])
      ++_size;
    _posSize = _size;
  }
  if (nxtBuf && !_nxtBufSize) {
    long nxtBufSize = 0;
    while (nxtBuf[nxtBufSize])
      ++nxtBufSize;
    _nxtBufSize = nxtBufSize;
    _size += nxtBufSize;
  }
  return _size;
}
http::webStr::webStr(char *pos_, char *nxtBuf_) {
  if (!pos_)
    throw std::logic_error("pos_ can't be NULL");
  pos = pos_;
  nxtBuf = nxtBuf_;
  _size = 0;
  _posSize = 0;
  _nxtBufSize = 0;
  size();
}
http::webStr::operator std::string() const {
  std::string result = (pos ? std::string(pos) : "");
  if (nxtBuf)
    result += std::string(nxtBuf);
  return result;
}
char http::webStr::operator[](size_t index) const {
  if (index < static_cast<size_t>(_posSize)) {
    return pos[index];
  } else if (nxtBuf && index < static_cast<size_t>(_size)) {
    return nxtBuf[index - _posSize];
  }
  throw std::out_of_range("Index out of range in webStr");
}
bool http::webStr::operator==(const webStr &other) const {
  if (size() != other.size())
    return false;
  for (long i = 0; i < size(); ++i) {
    if ((*this)[i] != other[i])
      return false;
  }
  return true;
}
bool http::webStr::operator==(const char *str) const {
  if (!str)
    return false;
  size_t i = 0;
  while (i < static_cast<size_t>(size()) && str[i]) {
    if ((*this)[i] != str[i])
      return false;
    ++i;
  }
  return i == static_cast<size_t>(size()) && str[i] == '\0';
}
bool operator==(const char *str, const http::webStr &ws) { return ws == str; }
bool http::webStr::operator==(const std::string &rhs) const {
  return std::string(*this) == rhs;
}
bool operator==(const std::string &lhs, const http::webStr &rhs) {
  return rhs == lhs;
}
bool http::webStr::operator!=(const webStr &other) const {
  return !(*this == other);
}
bool http::webStr::operator!=(const std::string &rhs) const {
  return !(*this == rhs);
}
bool http::webStr::operator!=(const char *rhs) const { return !(*this == rhs); }
bool operator!=(const char *lhs, const http::webStr &rhs) {
  return !(rhs == lhs);
}

// Reload of << operator
std::ostream &http::operator<<(std::ostream &os, const webStr &str) {
  if (str.pos) {
    os << str.pos;
  }
  if (str.nxtBuf) {
    os << str.nxtBuf;
  }
  return os;
}
// /webStr

// Request
http::Request::Request() : bodyLenth(0) {}
http::Request::~Request() {}

http::webStr http::Request::getHeader(char const *key) const {
  for (size_t i = 0; i < _headers.size(); ++i) {
    size_t key_len = std::strlen(_headers[i].first.pos);

    if (strncasecmp(_headers[i].first.pos, key, key_len) != 0)
      continue;
    if (_headers[i].first.nxtBuf && *(key + key_len) &&
        strcasecmp(_headers[i].first.nxtBuf, key + key_len) != 0)
      continue;
    else if (!_headers[i].first.nxtBuf &&
             strcasecmp(_headers[i].first.pos, key) != 0)
      continue;
    return _headers[i].second;
  }

  return webStr();
};
// /Request

// Connection
http::Connection::Connection(srvparams const &params)
    : curBuf(0), cursor(0), rnrnCounter(0), reqLen(0), params(params),
      bodyReadBytes(0), contentLength(0), bodyFd(-1), status(NOTHING_DONE) {
  buffers[0] = new webbuf(params.bufferSize);
  buffers[1] = new webbuf(params.bufferSize);
  bodyBuffer = new char[params.clientBodyBufferSize];
};

http::Connection::~Connection() {
  delete buffers[0];
  delete buffers[1];
  delete[] bodyBuffer;
}

void http::Connection::hndlIncStrm(int event_fd) {
  if (status == BODY_STARTED) {
    size_t bytesRead =
        read(event_fd, bodyBuffer,
             params.clientBodyBufferSize > contentLength - bodyReadBytes
                 ? contentLength - bodyReadBytes
                 : params.clientBodyBufferSize);
    size_t bytesWrote = write(bodyFd, bodyBuffer, bytesRead);
    if (bytesRead != bytesWrote)
      throw http::HttpError("couldn't read the body", 500);
    bodyReadBytes += bytesWrote;
    if (bodyReadBytes == contentLength) {
      status = ALL_DONE;
      close(bodyFd);
    }
    return;
  }

  if (status <= ALL_DONE) {
    buffers[curBuf]->readBuf(event_fd);
  }
  if (status >= ALL_DONE) {
    status = NOTHING_DONE;
    reqLen = 0;
    if (!curReq.getBodyPath().empty())
      std::remove(curReq.getBodyPath().c_str());
    curReq = Request();
  }

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
        if (status != VALUE && status != HTTPVERS) {
          throw http::HttpError(
              "The request must end with a header value or http version", 400);
        }
        *bgnRnrn = '\0';
        ++cursor;
        curReq._headers.push_back(
            std::pair<webStr, webStr>(webStr(buffer + cursor, NULL), webStr()));
        status = KEY;
        continue;
      }
      if (rnrnCounter == 3) {
        ++cursor;
        continue;
      }
      if (rnrnCounter == 4) {
        status = HEADERS_DONE;
        if (!curReq._headers.empty())
          curReq._headers.pop_back();
        ++cursor;
        break;
      }
    } else
      rnrnCounter = 0;
    switch (status) {
    case NOTHING_DONE:
      delim = ' ';
      if (iscntrl(buffer[cursor]) || isspace(buffer[cursor]))
        throw http::HttpError(
            "there can't be spaces at the beginning of the METHOD", 400);
      if (reqLen == 0)
        curReq.setMethod(buffer + cursor, NULL);
      break;
    case URI:
      delim = ' ';
      break;
    case HTTPVERS:
      delim = '\r';
      break;
    case KEY:
      if (iscntrl(buffer[cursor]) || isspace(buffer[cursor]))
        throw http::HttpError("key can't begin with a space", 400);
      ;
      delim = ':';
      break;
    case VALUE:
      if (iscntrl(buffer[cursor]) || isspace(buffer[cursor]))
        throw http::HttpError("value can't start with a space", 400);
      delim = '\r';
      break;
    default:;
    }
    while (buffer[cursor] && buffer[cursor] != delim) {
      if ((iscntrl(buffer[cursor]) || isspace(buffer[cursor])) &&
          status != VALUE)
        throw http::HttpError("you can't have a space before the delimeter",
                              400);
      ;
      ++cursor;
      ++reqLen;
      if (reqLen > params.bufferSize)
        throw http::HttpError("the request headers are too long", 400);
      ;
    }
    if (buffer[cursor] != delim) {
      curBuf = (curBuf + 1) % 2;
      cursor = 0;
      buffers[curBuf]->setCursor(0);
      switch (status) {
      case NOTHING_DONE:
        curReq.setMethod(NULL, buffers[curBuf]->getBuf());
        break;
      case URI:
        curReq.setUri(NULL, buffers[curBuf]->getBuf());
        break;
      case HTTPVERS:
        curReq.setHttpvers(NULL, buffers[curBuf]->getBuf());
        break;
      case KEY:
        curReq._headers.back().first.nxtBuf = buffers[curBuf]->getBuf();
        break;
      case VALUE:
        curReq._headers.back().second.nxtBuf = buffers[curBuf]->getBuf();
        break;
      default:;
      }
      return;
    }
    if (delim == ' ' || delim == ':') {
      buffer[cursor] = '\0';
      cursor += 1;
      ++reqLen;
      if (reqLen > params.bufferSize)
        throw http::HttpError("the request is too long", 400);
      ;
    } else {
      ++rnrnCounter;
      bgnRnrn = buffer + cursor;
      ++cursor;
      continue;
    }
    switch (status) {
    case NOTHING_DONE:
      if (iscntrl(buffer[cursor]) || isspace(buffer[cursor]))
        throw http::HttpError("URI must start with a character", 400);
      ;
      status = URI;
      curReq.setUri(buffer + cursor, NULL);
      break;
    case URI:
      if (iscntrl(buffer[cursor]) || isspace(buffer[cursor]))
        throw http::HttpError("httvers must start with a character", 400);
      ;
      status = HTTPVERS;
      curReq.setHttpvers(buffer + cursor, NULL);
      break;
    case KEY:
      if (buffer[cursor] != ' ')
        throw http::HttpError("there should be a space after a colon", 400);
      ;
      status = VALUE;
      ++cursor;
      ++reqLen;
      if (reqLen > params.bufferSize)
        throw std::runtime_error("400");
      if (iscntrl(buffer[cursor]) || isspace(buffer[cursor]))
        throw std::runtime_error("400");
      curReq._headers[curReq._headers.size() - 1].second =
          webStr(buffer + cursor, NULL);
      break;
    default:;
    }
    ++cursor;
    ++reqLen;
    if (reqLen > params.bufferSize)
      throw http::HttpError("the request is too long", 400);
    ;
  }
  if (status == HEADERS_DONE &&
      (curReq.getMethod() == "GET" || curReq.getMethod() == "DELETE")) {
    status = ALL_DONE;
    if (buffer[cursor])
      status = NEXT_REQUEST;
    return;
  }
  if (status == HEADERS_DONE) {
    webStr contentLengthRaw = curReq.getHeader("Content-Length");
    contentLength = utils::webStrToSizeT(contentLengthRaw);
    if (!contentLengthRaw.pos)
      throw http::HttpError("couldn't read content-length", 400);
    ;
    if (buffer[cursor]) {
      this->curReq.setBody(buffer + cursor);
      this->bodyReadBytes = this->buffers[curBuf]->getCursor() - this->cursor;
      this->cursor += bodyReadBytes;
      this->curReq.setBodyLength(bodyReadBytes);

      if (this->bodyReadBytes >= contentLength) {
        status = ALL_DONE;
        if (buffer[cursor])
          status = NEXT_REQUEST;
        return;
      }
    } else
      curReq.setBody(NULL);
    status = BODY_STARTED;

    std::ostringstream oss;
    oss << event_fd;
    std::string event_fd_str = oss.str();

    if (this->buffers[curBuf]->getCursor() == this->buffers[curBuf]->getSize())
      this->cursor = 0;
    std::string filePath = "tmp/" + event_fd_str;
    curReq.setBodyPath(filePath);
    std::cout << "Set Body Path: " << filePath << std::endl;
    if (bodyFd == -1) {
      bodyFd = open(filePath.c_str(), O_CREAT | O_WRONLY, 0644);
    }
    ftruncate(bodyFd, 0);
  }
}
// /Connection

http::Response::Response(srvparams const &params, int port_)
    : protocol(params.protocol), port(port_), statusCode(http::OK),
      statusMessage("OK") {}
http::Response::~Response() {
  for (std::map<std::string, void *>::iterator it = hooksMap.begin();
       it != hooksMap.end(); ++it) {
    if (it->second != NULL && deleters[it->first] != NULL) {
      deleters[it->first](it->second);
    }
  }
}
void http::Response::setStatusCode(int statusCode_) {
  statusCode = statusCode_;
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
std::string http::Response::getBody() const { return this->body; }
int http::Response::getPort() const { return this->port; }

std::string http::Request::getBodyPath() const { return this->bodyPath; }

char *http::Request::getBody() const { return body; }
