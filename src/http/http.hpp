#pragma once

#include "../server/server.hpp"
#include <cstddef>
#include <cstring>
#include <exception>
#include <utility>
#include <vector>

namespace http {
enum RequestStatus {
  NOTHING_DONE,
  REQUEST_LINE_DONE = 0,
  HEADERS_DONE,
  BODY_DONE,
  ALL_DONE
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
  size_t end;
  bool full;

public:
  void setFull();
  bool isFull();
  void readBuf(int fd);
  size_t getSize();
  size_t getEnd();
  void setEnd(int end);
  char *getBuffer();
  webbuf(int size);
  ~webbuf();
};
// /Buffer

// Request
class Request {
private:
  char *method;
  bool methodParsed;
  char *uri;
  bool uriParsed;
  char *httpvers;
  bool httpversParsed;
  // clang-format off
  std::vector<std::pair<char *, char *> > headers;
  // clang-format on
  std::vector<webbuf> header_buffers;
  size_t currbuff;
  size_t cursor;
  int endseq_cnt;
  // Utils
  RequestStatus status;
  // /Utils

public:
  void handleData(int fd, srvparams const &params);
  int line_len;
  Request(srvparams const &params);
  Request &operator=(Request const &req);
  // TODO: Delete buffers
  ~Request();
};
// /Request
}; // namespace http
