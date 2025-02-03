#pragma once

#include "../server/server.hpp"
#include <cstring>
#include <exception>
#include <string>

namespace http {
enum RequestStatus {
  NOTHING_DONE,
  REQUEST_LINE_DONE = 0,
  HEADERS_DONE,
  BODY_DONE,
  ALL_DONE
};

// Statuses
static const char *Continue = "100";
static const char *SwitchingProtocols = "101";
static const char *Processing = "102";
static const char *EarlyHints = "103";

// Successful responses
static const char *OK = "200";
static const char *Created = "201";
static const char *Accepted = "202";
static const char *NonAuthoritativeInformation = "203";
static const char *NoContent = "204";
static const char *ResetContent = "205";
static const char *PartialContent = "206";
static const char *MultiStatus = "207";
static const char *AlreadyReported = "208";
static const char *IMUsed = "226";

// Redirection messages
static const char *MultipleChoices = "300";
static const char *MovedPermanently = "301";
static const char *Found = "302";
static const char *SeeOther = "303";
static const char *NotModified = "304";
static const char *UseProxy = "305"; // Deprecated
static const char *Unused = "306";   // Reserved
static const char *TemporaryRedirect = "307";
static const char *PermanentRedirect = "308";

// Client error responses
static const char *BadRequest = "400";
static const char *Unauthorized = "401";
static const char *PaymentRequired = "402";
static const char *Forbidden = "403";
static const char *NotFound = "404";
static const char *MethodNotAllowed = "405";
static const char *NotAcceptable = "406";
static const char *ProxyAuthenticationRequired = "407";
static const char *RequestTimeout = "408";
static const char *Conflict = "409";
static const char *Gone = "410";
static const char *LengthRequired = "411";
static const char *PreconditionFailed = "412";
static const char *ContentTooLarge = "413";
static const char *URITooLong = "414";
static const char *UnsupportedMediaType = "415";
static const char *RangeNotSatisfiable = "416";
static const char *ExpectationFailed = "417";
static const char *ImATeapot = "418";
static const char *MisdirectedRequest = "421";
static const char *UnprocessableContent = "422";
static const char *Locked = "423";
static const char *FailedDependency = "424";
static const char *TooEarly = "425";
static const char *UpgradeRequired = "426";
static const char *PreconditionRequired = "428";
static const char *TooManyRequests = "429";
static const char *RequestHeaderFieldsTooLarge = "431";
static const char *UnavailableForLegalReasons = "451";

// Server error responses
static const char *InternalServerError = "500";
static const char *NotImplemented = "501";
static const char *BadGateway = "502";
static const char *ServiceUnavailable = "503";
static const char *GatewayTimeout = "504";
static const char *HTTPVersionNotSupported = "505";
static const char *VariantAlsoNegotiates = "506";
static const char *InsufficientStorage = "507";
static const char *LoopDetected = "508";
static const char *NotExtended = "510";
static const char *NetworkAuthenticationRequired = "511";
// /Statuses

class HttpError : public std::exception {
private:
  char message[256]; // Fixed-size buffer to store the error message

public:
  HttpError(const char *msg) {
    std::strncpy(message, msg, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
  }
  const char *what() const throw() { return message; }
};
// Buffer
class webbuf {
private:
  char *buffer;
  int size;
  int end;
  bool full;

public:
  void setFull();
  bool isFull();
  void readBuf(int fd);
  int getSize();
  int getEnd();
  void setEnd(int end);
  char *getBuffer();
  webbuf(int size);
  ~webbuf();
};
// /Buffer

// Request
class Request {
private:
  std::string method;
  std::string uri;
  std::string http_version;
  // Utils
  RequestStatus parsing_status;
  // /Utils

public:
  void handleData(int fd, srvparams const &params);
  RequestStatus buffers_status;
  int request_line_len;
  webbuf header_buffer;
  Request(srvparams const &params);
  Request &operator=(Request const &req);
  // TODO: Delete buffers
  ~Request();
};
// /Request
}; // namespace http
