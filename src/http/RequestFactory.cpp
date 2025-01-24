#include "RequestFactory.hpp"

/**
 * RequestFactory
 */
bool RequestFactory::ifExists(int fd) {
  return requests.end() != requests.find(fd);
}
Request &RequestFactory::getRequest(int fd) {
  if (ifExists(fd))
    return requests[fd];
  throw std::runtime_error("Request not found");
}
void RequestFactory::setRequest(Request const &req, int fd) {
  if (!ifExists(fd))
    requests[fd] = req;
}
void RequestFactory::deleteRequest(int fd) { requests.erase(fd); }

RequestFactory::RequestFactory() {}
RequestFactory::~RequestFactory() {}
/**
 * /RequestFactory
 */
