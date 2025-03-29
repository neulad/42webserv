#include "utils.hpp"
#include <arpa/inet.h>

size_t utils::webStrToSizeT(const http::webStr &wstr) {
  std::string combinedStr;

  if (wstr.pos) {
    combinedStr += wstr.pos;
  }
  if (wstr.nxtBuf) {
    combinedStr += wstr.nxtBuf;
  }
  return std::strtoull(combinedStr.c_str(), NULL, 10);
}

bool utils::matchEndpoint(const std::string &endpoint,
                          const http::webStr &uri) {
  size_t starPos = endpoint.find('*');

  // Convert webStr to a single std::string for easier comparison
  std::string fullUri(uri.pos);
  if (uri.nxtBuf) {
    fullUri += uri.nxtBuf;
  }

  // Remove query string part (anything after '?')
  size_t queryPos = fullUri.find('?');
  if (queryPos != std::string::npos) {
    fullUri = fullUri.substr(0, queryPos);
  }

  if (starPos == std::string::npos) {
    // Compare endpoint with the cleaned uri (without query string)
    return endpoint == fullUri;
  }

  std::string beforeStar = endpoint.substr(0, starPos);
  std::string afterStar = endpoint.substr(starPos + 1);

  if (fullUri.size() < beforeStar.size() + afterStar.size()) {
    return false; // URI is too short to match
  }

  if (fullUri.substr(0, beforeStar.size()) == beforeStar &&
      fullUri.substr(fullUri.size() - afterStar.size()) == afterStar) {
    return true;
  }

  return false;
}

std::string utils::getHttpStatusMessage(int statusCode) {
  // Define a map with all official HTTP status codes and their messages
  static const std::pair<int, const char *> statusMessages[] = {
      // 1xx: Informational
      std::make_pair(100, "Continue"),
      std::make_pair(101, "Switching Protocols"),
      std::make_pair(102, "Processing"), std::make_pair(103, "Early Hints"),

      // 2xx: Success
      std::make_pair(200, "OK"), std::make_pair(201, "Created"),
      std::make_pair(202, "Accepted"),
      std::make_pair(203, "Non-Authoritative Information"),
      std::make_pair(204, "No Content"), std::make_pair(205, "Reset Content"),
      std::make_pair(206, "Partial Content"),
      std::make_pair(207, "Multi-Status"),
      std::make_pair(208, "Already Reported"), std::make_pair(226, "IM Used"),

      // 3xx: Redirection
      std::make_pair(300, "Multiple Choices"),
      std::make_pair(301, "Moved Permanently"), std::make_pair(302, "Found"),
      std::make_pair(303, "See Other"), std::make_pair(304, "Not Modified"),
      std::make_pair(305, "Use Proxy"),
      std::make_pair(307, "Temporary Redirect"),
      std::make_pair(308, "Permanent Redirect"),

      // 4xx: Client Errors
      std::make_pair(400, "Bad Request"), std::make_pair(401, "Unauthorized"),
      std::make_pair(402, "Payment Required"), std::make_pair(403, "Forbidden"),
      std::make_pair(404, "Not Found"),
      std::make_pair(405, "Method Not Allowed"),
      std::make_pair(406, "Not Acceptable"),
      std::make_pair(407, "Proxy Authentication Required"),
      std::make_pair(408, "Request Timeout"), std::make_pair(409, "Conflict"),
      std::make_pair(410, "Gone"), std::make_pair(411, "Length Required"),
      std::make_pair(412, "Precondition Failed"),
      std::make_pair(413, "Payload Too Large"),
      std::make_pair(414, "URI Too Long"),
      std::make_pair(415, "Unsupported Media Type"),
      std::make_pair(416, "Range Not Satisfiable"),
      std::make_pair(417, "Expectation Failed"),
      std::make_pair(418, "I'm a Teapot"), // Fun Easter egg in RFC 2324
      std::make_pair(421, "Misdirected Request"),
      std::make_pair(422, "Unprocessable Entity"),
      std::make_pair(423, "Locked"), std::make_pair(424, "Failed Dependency"),
      std::make_pair(425, "Too Early"), std::make_pair(426, "Upgrade Required"),
      std::make_pair(428, "Precondition Required"),
      std::make_pair(429, "Too Many Requests"),
      std::make_pair(431, "Request Header Fields Too Large"),
      std::make_pair(451, "Unavailable For Legal Reasons"),

      // 5xx: Server Errors
      std::make_pair(500, "Internal Server Error"),
      std::make_pair(501, "Not Implemented"),
      std::make_pair(502, "Bad Gateway"),
      std::make_pair(503, "Service Unavailable"),
      std::make_pair(504, "Gateway Timeout"),
      std::make_pair(505, "HTTP Version Not Supported"),
      std::make_pair(506, "Variant Also Negotiates"),
      std::make_pair(507, "Insufficient Storage"),
      std::make_pair(508, "Loop Detected"), std::make_pair(510, "Not Extended"),
      std::make_pair(511, "Network Authentication Required")};

  // Find the status code in the array
  for (size_t i = 0; i < sizeof(statusMessages) / sizeof(statusMessages[0]);
       ++i) {
    if (statusMessages[i].first == statusCode) {
      std::stringstream ss;
      ss << statusMessages[i].second;
      return ss.str();
    }
  }

  // If the status code is unknown, return a generic message
  return "Unknown Status Code";
}

int utils::getPortNumber(int fd) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);

  if (getsockname(fd, (struct sockaddr *)&addr, &addr_len) == -1) {
    return -1; // Return -1 on failure
  }

  return ntohs(
      addr.sin_port); // Convert from network byte order to host byte order
}

bool utils::startsWith(const std::string &prefix, const std::string &str) {
  if (str.length() < prefix.length()) {
    return false; // str is shorter than prefix, cannot be a match
  }
  return str.substr(0, prefix.length()) == prefix;
}
std::string utils::getMimeType(const std::string &filename) {
  static std::map<std::string, std::string> mimeTypes;
  if (mimeTypes.empty()) {
    mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".csv"] = "text/csv";

    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".bmp"] = "image/bmp";
    mimeTypes[".ico"] = "image/x-icon";
    mimeTypes[".svg"] = "image/svg+xml";

    mimeTypes[".mp3"] = "audio/mpeg";
    mimeTypes[".wav"] = "audio/wav";
    mimeTypes[".ogg"] = "audio/ogg";
    mimeTypes[".mp4"] = "video/mp4";
    mimeTypes[".webm"] = "video/webm";
    mimeTypes[".avi"] = "video/x-msvideo";
    mimeTypes[".mov"] = "video/quicktime";
    mimeTypes[".flv"] = "video/x-flv";

    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".zip"] = "application/zip";
    mimeTypes[".tar"] = "application/x-tar";
    mimeTypes[".gz"] = "application/gzip";
    mimeTypes[".rar"] = "application/vnd.rar";
    mimeTypes[".7z"] = "application/x-7z-compressed";
    mimeTypes[".iso"] = "application/x-iso9660-image";
    mimeTypes[".exe"] = "application/x-msdownload";
    mimeTypes[".msi"] = "application/x-ms-installer";
    mimeTypes[".deb"] = "application/vnd.debian.binary-package";
    mimeTypes[".rpm"] = "application/x-rpm";

    mimeTypes[".ttf"] = "font/ttf";
    mimeTypes[".otf"] = "font/otf";
    mimeTypes[".woff"] = "font/woff";
    mimeTypes[".woff2"] = "font/woff2";
  }

  // Find the last dot in the filename
  std::size_t dotPos = filename.rfind('.');
  if (dotPos != std::string::npos) {
    std::string ext = filename.substr(dotPos);
    if (mimeTypes.find(ext) != mimeTypes.end())
      return mimeTypes[ext];
  }
  return "application/octet-stream"; // Default for unknown files
}

std::string utils::intToString(int num) {
  std::stringstream ss;
  ss << num;       // Insert the integer into the stream
  return ss.str(); // Convert stream to string
}
