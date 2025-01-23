#include "http.hpp"

#include <cctype>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>

/**
 * Utility functions
 */
std::string http::readLine(const std::string &raw_req, int &cursor) {
  int start = cursor;
  int end = raw_req.find('\n', start);
  if ((unsigned long)end == std::string::npos) {
    end = raw_req.size();
  }
  std::string line = raw_req.substr(start, end - start);
  cursor = end + 1;
  return line;
}
bool http::onlySpaces(const std::string &str) {
  for (size_t i = 0; i < str.length(); ++i)
    if (!std::isspace(str[i]))
      return false;
  return true;
}
std::vector<std::string> http::splitBySpace(const std::string &line) {
  std::vector<std::string> tokens;
  std::istringstream stream(line);
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}
std::string http::trim(std::string const &str) {
  int start = 0;
  int end = str.size() - 1;

  while (start <= end && std::isspace(str[start]))
    ++start;
  while (end >= start && std::isspace(str[end]))
    --end;

  return str.substr(start, end - start + 1);
}
std::vector<std::string> http::splitByColon(const std::string &line) {
  std::vector<std::string> tokens;
  size_t pos = line.find(':');

  if (pos != std::string::npos) {
    std::string name = trim(line.substr(0, pos));
    std::string value = trim(line.substr(pos + 1));

    tokens.push_back(name);
    tokens.push_back(value);
  }

  return tokens;
}

std::string join(const std::vector<std::string> &strs, char delim,
                 unsigned int start, unsigned int end) {
  if (start >= strs.size())
    return "";
  if (end <= start || end > strs.size())
    return "";

  std::ostringstream oss;
  for (unsigned int i = start; i < end; ++i) {
    if (i > start)
      oss << delim;
    oss << strs[i];
  }

  return oss.str();
}

std::string http::toLowerCaseCopy(const std::string &str) {
  std::string lowerStr = str;
  for (size_t i = 0; i < lowerStr.size(); ++i) {
    lowerStr[i] = std::tolower(lowerStr[i]);
  }
  return lowerStr;
}
/**
 * /Utility functions
 */

/**
 * System
 */
http::request::request(const std::string &raw_req) {
  int cursor = 0;
  std::string line = readLine(raw_req, cursor);

  std::vector<std::string> tokens = splitBySpace(line);
  if (tokens.size() < 3)
    throw std::invalid_argument("request-line is too short");

  this->method = tokens[0];
  this->url = tokens[1];
  this->protocol = tokens[2];

  while ((unsigned long)cursor < raw_req.size()) {
    line = readLine(raw_req, cursor);
    if (line.empty() || onlySpaces(line))
      break;

    std::vector<std::string> tokens = splitByColon(line);
    if (tokens.size() != 2)
      throw std::invalid_argument("malformed header");

    this->headers.push_back(std::make_pair(tokens[0], tokens[1]));
  }
  while ((unsigned long)cursor < raw_req.size()) {
    line = readLine(raw_req, cursor);
    if (line.empty())
      continue;
    this->body += line;
  }
}
http::request::~request() {}
/**
 * /System
 */

/**
 * Logic
 */
std::string http::request::getMethod() const { return this->method; }
std::string http::request::getProtocol() const { return this->protocol; }
std::string http::request::getUrl() const { return this->url; }
const std::string &http::request::getBody() const { return this->body; }
std::string http::request::getHeader(std::string const &name) const {
  for (size_t i = 0; i < this->headers.size(); ++i) {
    if (http::toLowerCaseCopy(this->headers[i].first) ==
        http::toLowerCaseCopy(name))
      return this->headers[i].second;
  }
  return "";
}
// clang-format off
const std::vector<std::pair<std::string, std::string> > &
// clang-format on
http::request::getAllHeaders() const {
  return headers;
}
/**
 * /Logic
 */
