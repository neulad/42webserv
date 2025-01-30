#include "RequestFactory.hpp"
#include <stdexcept>

/**
 * RequestFactory
 */
bool RequestFactory::ifExists(int fd) {
  return requests.end() != requests.find(fd);
}
server::Request &RequestFactory::getRequest(int fd) {
  if (ifExists(fd))
    return requests[fd];
  throw std::runtime_error("Request not found");
}
void RequestFactory::setRequest(server::Request const &req, int fd) {
  if (!ifExists(fd))
    requests[fd] = req;
}
void RequestFactory::deleteRequest(int fd) { requests.erase(fd); }

RequestFactory::RequestFactory() {}
RequestFactory::~RequestFactory() {}
/**
 * /RequestFactory
 */
