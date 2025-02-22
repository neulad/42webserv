#include "utils.hpp"
#include <cstddef>
#include <cstring>

bool utils::cmpWebStrs(http::webStr str1, http::webStr str2) {
  const char *ptr1 = str1.pos;
  const char *ptr2 = str2.pos;

  while (ptr1 && ptr2) {
    while (*ptr1 && *ptr2) {
      if (*ptr1 != *ptr2)
        return false;
      ++ptr1;
      ++ptr2;
    }
    if (*ptr1 == '\0' && str1.nxtBuf) {
      ptr1 = str1.nxtBuf;
      str1.nxtBuf = NULL;
    }
    if (*ptr2 == '\0' && str2.nxtBuf) {
      ptr2 = str2.nxtBuf;
      str2.nxtBuf = NULL;
    }
    if (*ptr1 == '\0' && *ptr2 == '\0')
      return true;
  }
  return (*ptr1 == '\0' && *ptr2 == '\0');
}

bool utils::cmpWebStrs(http::webStr str1, char *str2) {
  const char *ptr1 = str1.pos;
  while (ptr1 && str2) {
    while (*ptr1 && *str2) {
      if (*ptr1 != *str2)
        return false;
      ++ptr1;
      ++str2;
    }
    if (*ptr1 == '\0' && str1.nxtBuf) {
      ptr1 = str1.nxtBuf;
      str1.nxtBuf = NULL;
    }
    if (*ptr1 == '\0' && *str2 == '\0')
      return true;
  }
  return (*ptr1 == '\0' && *str2 == '\0');
}

bool utils::cmpWebStrs(char *str1, http::webStr str2) {
  return cmpWebStrs(str2, str1);
}
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

  if (starPos == std::string::npos) {
    // Compare endpoint with the whole uri
    const char *uriPtr = uri.pos;
    if (uri.nxtBuf) {
      std::string fullUri(uri.pos);
      fullUri += uri.nxtBuf;
      return endpoint == fullUri;
    }
    return endpoint == uriPtr;
  }

  std::string beforeStar = endpoint.substr(0, starPos);
  std::string afterStar = endpoint.substr(starPos + 1);

  // Convert webStr to a single std::string for easier comparison
  std::string fullUri(uri.pos);
  if (uri.nxtBuf) {
    fullUri += uri.nxtBuf;
  }

  if (fullUri.size() < beforeStar.size() + afterStar.size()) {
    return false; // URI is too short to match
  }

  if (fullUri.substr(0, beforeStar.size()) == beforeStar &&
      fullUri.substr(fullUri.size() - afterStar.size()) == afterStar) {
    return true;
  }

  return false;
}
