#include "request.hpp"

/**
 * RequestFactory
 */
bool RequestFactory::ifExists(int fd) {
  return requests.end() != requests.find(fd);
}

const Request &RequestFactory::getRequest(int fd) {
  if (ifExists(fd))
    return requests[fd];
  throw std::runtime_error("Request not found");
}

RequestFactory::RequestFactory() {}
RequestFactory::~RequestFactory() {}
/**
 * /RequestFactory
 */

/**
 * Request
 */
// Request::Request(int fd) {}
/**
 * /Request
 */
