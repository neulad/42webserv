#include "http.hpp"

#include <stdexcept>

std::string readLine(std::string const &str) {
  std::string line;
  for (int i = 0; i < str.length(); ++i) {
    if (str[i] == '\n') return line;
    line += str[i];
  }
  return line;
}

std::vector<std::string> split(const std::string &str, char delimiter) {
  std::vector<std::string> result;
  std::string word;

  for (size_t i = 0; i < str.length(); ++i) {
    char ch = str[i];
    if (ch == delimiter) {
      if (!word.empty()) {
        result.push_back(word);
        word.clear();
      }
    } else {
      word += ch;
    }
  }
  if (!word.empty()) {
    result.push_back(word);
  }
  return result;
}

http::request::request(std::string const &raw_req) {
  std::string line = readLine(raw_req);
  int cursor = raw_req.find(' ');
  std::vector<std::string> tokens = split(line, ' ');
  if (tokens.size() < 3) {
    throw std::invalid_argument("request-line is too short");
  }
  this->method = tokens[0];
  this->url = tokens[1];
  this->protocol = tokens[2];
}
