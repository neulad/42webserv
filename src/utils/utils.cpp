#include "utils.hpp"
#include <cstddef>
#include <cstring>
#include <sstream>

std::vector<std::string> utils::splitBeforeNewline(std::string const &str,
                                                   size_t const cursor) {
  if (cursor == str.size())
    return std::vector<std::string>();
  std::vector<std::string> result;
  size_t newline_pos = str.find(cursor, '\n');
  std::string part = str.substr(cursor, newline_pos);

  std::istringstream iss(part);
  std::string word;
  while (iss >> word) {
    result.push_back(word);
  }

  return result;
}

bool utils::iCompare(const std::string &str1, const std::string &str2) {
  if (str1.length() != str2.length()) {
    return false;
  }
  for (size_t i = 0; i < str1.length(); ++i) {
    if (tolower(str1[i]) != tolower(str2[i])) {
      return false;
    }
  }
  return true;
}

bool utils::seqPresent(char const *seq, ...) {
  va_list args;
  va_start(args, seq);

  char *curr;
  int reps = strlen(seq);
  int count = 0;
  while ((curr = va_arg(args, char *)) != NULL) {
    while (*curr) {
      if (seq[count] == *curr++)
        ++count;
      else
        count = 0;
      if (count == reps)
        return va_end(args), true;
    }
  }
  va_end(args);
  return false;
}

bool utils::anyPresent(char const *any, char *str) {
  size_t anylen = strlen(any);
  while (*str)
    for (size_t i = 0; i < anylen; ++i) {
      if (any[i] == *str)
        return true;
      ++str;
    }
  return false;
}

bool matchEndpoint(const std::string &endpoint, const std::string &uri) {
  size_t starPos = endpoint.find('*');

  if (starPos == std::string::npos) {
    return endpoint == uri;
  }

  std::string beforeStar = endpoint.substr(0, starPos);
  std::string afterStar = endpoint.substr(starPos + 1);

  if (uri.size() < beforeStar.size() + afterStar.size()) {
    return false; // URI is too short to match
  }

  if (uri.substr(0, beforeStar.size()) == beforeStar &&
      uri.substr(uri.size() - afterStar.size()) == afterStar) {
    return true;
  }

  return false;
}
