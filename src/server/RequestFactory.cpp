#include "RequestFactory.hpp"
#include <stdexcept>

/**
 * RequestFactory
 */
bool RequestFactory::ifExists(int fd) {
  return requests.end() != requests.find(fd);
}
http::Request &RequestFactory::getRequest(int fd) {
  if (ifExists(fd))
    return *requests[fd];
  throw std::runtime_error("Request not found");
}
void RequestFactory::setRequest(http::Request *req, int fd) {
  if (!ifExists(fd))
    requests[fd] = req;
}
void RequestFactory::deleteRequest(int fd) { requests.erase(fd); }

RequestFactory::RequestFactory() {}
RequestFactory::~RequestFactory() {
  for (size_t i = 0; i < this->requests.size(); ++i)
    delete this->requests[i];
}
/**
 * /RequestFactory
 */
