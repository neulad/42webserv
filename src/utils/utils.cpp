#include "utils.hpp"
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