#pragma once

#include "../server/FilefdFactory.hpp"
#include "../server/server.hpp"
#include <cstddef>
#include <cstring>
#include <exception>
#include <map>
#include <sstream>
#include <string>

namespace http {
enum RequestStatus {
  NOTHING_DONE = 0,
  URI,
  HTTPVERS,
  KEY,
  VALUE,
  HEADERS_DONE,
  BODY_STARTED,
  ALL_DONE,
  NEXT_REQUEST
};

// Statuses
static const int Continue = 100;
static const int SwitchingProtocols = 101;
static const int Processing = 102;
static const int EarlyHints = 103;

// Successful responses
static const int OK = 200;
static const int Created = 201;
static const int Accepted = 202;
static const int NonAuthoritativeInformation = 203;
static const int NoContent = 204;
static const int ResetContent = 205;
static const int PartialContent = 206;
static const int MultiStatus = 207;
static const int AlreadyReported = 208;
static const int IMUsed = 226;

// Redirection messages
static const int MultipleChoices = 300;
static const int MovedPermanently = 301;
static const int Found = 302;
static const int SeeOther = 303;
static const int NotModified = 304;
static const int UseProxy = 305; // Deprecated
static const int Unused = 306;   // Reserved
static const int TemporaryRedirect = 307;
static const int PermanentRedirect = 308;

// Client error responses
static const int BadRequest = 400;
static const int Unauthorized = 401;
static const int PaymentRequired = 402;
static const int Forbidden = 403;
static const int NotFound = 404;
static const int MethodNotAllowed = 405;
static const int NotAcceptable = 406;
static const int ProxyAuthenticationRequired = 407;
static const int RequestTimeout = 408;
static const int Conflict = 409;
static const int Gone = 410;
static const int LengthRequired = 411;
static const int PreconditionFailed = 412;
static const int ContentTooLarge = 413;
static const int URITooLong = 414;
static const int UnsupportedMediaType = 415;
static const int RangeNotSatisfiable = 416;
static const int ExpectationFailed = 417;
static const int ImATeapot = 418;
static const int MisdirectedRequest = 421;
static const int UnprocessableContent = 422;
static const int Locked = 423;
static const int FailedDependency = 424;
static const int TooEarly = 425;
static const int UpgradeRequired = 426;
static const int PreconditionRequired = 428;
static const int TooManyRequests = 429;
static const int RequestHeaderFieldsTooLarge = 431;
static const int UnavailableForLegalReasons = 451;

// Server error responses
static const int InternalServerError = 500;
static const int NotImplemented = 501;
static const int BadGateway = 502;
static const int ServiceUnavailable = 503;
static const int GatewayTimeout = 504;
static const int HTTPVersionNotSupported = 505;
static const int VariantAlsoNegotiates = 506;
static const int InsufficientStorage = 507;
static const int LoopDetected = 508;
static const int NotExtended = 510;
static const int NetworkAuthenticationRequired = 511;
// /Statuses

class HttpError : public std::exception {
private:
  char message[256]; // Fixed-size buffer to store the error message
  int statusCode;

public:
  HttpError(const char *msg, int statusCode) {
    std::strncpy(message, msg, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
    this->statusCode = statusCode;
  }
  const char *what() const throw() { return message; }
  int getStatus() const { return statusCode; }
};

// Buffer
class webbuf {
private:
  char *buffer;
  size_t size;
  size_t cursor;
  size_t length;

public:
  bool isFull();
  void readBuf(int fd);
  size_t getSize();
  size_t getCursor();
  void setCursor(size_t newCursor);
  char *getBuf();
  webbuf(int size);
  ~webbuf();
};
// /Buffer

// webStr
class webStr {
private:
  mutable long _size;
  mutable long _posSize;
  mutable long _nxtBufSize;

public:
  char *pos;
  char *nxtBuf;
  // Size function, now correctly marked const
  long size() const;

  // Constructors
  webStr() : _size(0), _posSize(0), _nxtBufSize(0), pos(NULL), nxtBuf(NULL) {}
  webStr(char *pos_, char *nxtBuf_);

  operator std::string() const;
  char operator[](size_t index) const;
  bool operator==(const webStr &other) const;
  bool operator==(const char *str) const;
  friend bool operator==(const char *str, const webStr &ws);
  bool operator==(const std::string &rhs) const;
  friend bool operator==(const std::string &lhs, const webStr &rhs);

  // Inequality operators
  bool operator!=(const webStr &other) const;
  bool operator!=(const std::string &rhs) const;
  bool operator!=(const char *rhs) const;
  friend bool operator!=(const char *lhs, const webStr &rhs);
};

std::ostream &operator<<(std::ostream &os, const webStr &str);
// /webStr

// Request
class Request {
private:
  webStr _uri;
  webStr _method;
  webStr _httpvers;
  char *body;
  size_t bodyLenth;
  std::string bodyPath;

public:
  // clang-format off
  std::vector<std::pair<webStr, webStr> > _headers;
  // clang-format on
  std::string getBodyPath() const;
  size_t getBodyLength() const { return bodyLenth; }
  void setBodyPath(std::string path) { bodyPath = path; };
  webStr getHeader(char const *key) const;
  webStr getUri() const { return _uri; };
  webStr getMethod() const { return _method; };
  webStr getHttpvers() const { return _httpvers; };
  void setMethod(char *pos_, char *nxtBuf_) {
    if (pos_)
      _method.pos = pos_;
    if (nxtBuf_)
      _method.nxtBuf = nxtBuf_;
  }
  void setBodyLength(size_t length) { this->bodyLenth = length; }
  void setUri(char *pos_, char *nxtBuf_) {
    if (pos_)
      _uri.pos = pos_;
    if (nxtBuf_)
      _uri.nxtBuf = nxtBuf_;
  }
  void setHttpvers(char *pos_, char *nxtBuf_) {
    if (pos_)
      _httpvers.pos = pos_;
    if (nxtBuf_)
      _httpvers.nxtBuf = nxtBuf_;
  }
  void setHeader(webStr key, webStr value) {
    _headers.push_back(std::pair<webStr, webStr>(key, value));
  }
  void setBody(char *body_) { body = body_; }
  char *getBody() const;
  Request();
  ~Request();
};
// /Request

// Connection
class Connection {
private:
  Request curReq;
  char *bodyBuffer;
  size_t curBuf;
  webbuf *buffers[2];
  int cursor;
  int rnrnCounter;
  int reqLen;
  srvparams params;
  size_t bodyReadBytes;
  size_t contentLength;
  int bodyFd;

public:
  RequestStatus status;
  void hndlIncStrm(int event_fd);
  Request &getReq() { return curReq; };
  Connection(srvparams const &params);
  ~Connection();
};
// /Connection

// Response
class Response {
private:
  std::string const protocol;
  int port;
  int statusCode;
  std::string statusMessage;
  // clang-format off
  std::vector<std::pair<std::string, std::string> > headers;
  // clang-format on
  std::string body;
  std::string bodyPath;
  std::ostringstream response;
  std::map<std::string, void *> hooksMap;
  std::map<std::string, void (*)(void *)> deleters;
  template <typename T> static void deleteObject(void *ptr) {
    delete static_cast<T *>(ptr);
  }

public:
  bool done;
  void end(int fd, FilefdFactory &filefdfaq);

  // Getters/Setters
  void setStatusCode(int const statusCode);
  void setStatusMessage(std::string const statusMessage);
  void setHeader(std::string key, std::string value);
  void setBodyPath(std::string const bodyPath);
  void setBody(std::string const &body);
  std::string getBody() const;
  int getPort() const;

  bool isBodyReady();
  template <typename T> void setHookMap(std::string key, T *value) {
    hooksMap[key] = value;
    deleters[key] = &deleteObject<T>;
  };
  template <typename T> T *getHookMap(std::string key) {
    return static_cast<T *>(hooksMap[key]);
  };
  Response(srvparams const &params, int port);
  ~Response();
};
// /Response

}; // namespace http
