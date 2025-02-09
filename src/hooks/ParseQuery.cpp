#include "ParseQuery.hpp"

void parseQueryString(http::Request const &req, http::Response &res) {
  // Extract the URI from the request
  const std::string &uri = req.getUri();

  // Find the position of the query string
  size_t queryStart = uri.find('?');
  if (queryStart == std::string::npos) {
    res.setHookMap<queryStringType>("queryString", new queryStringType());
    return;
  }

  // Extract the query string part
  std::string queryString = uri.substr(queryStart + 1);

  // Create a map to hold key-value pairs
  queryStringType *queryMap = new queryStringType();

  // Parse the query string
  std::istringstream queryStream(queryString);
  std::string pair;
  while (std::getline(queryStream, pair, '&')) {
    size_t delimiterPos = pair.find('=');
    std::string key, value;

    if (delimiterPos != std::string::npos) {
      key = pair.substr(0, delimiterPos);
      value = pair.substr(delimiterPos + 1);
    } else {
      key = pair;
      value = ""; // No value for the key
    }

    (*queryMap)[key] = value;
  }

  // Add the map to the response using setHookMap
  res.setHookMap("queryString", queryMap);
}
