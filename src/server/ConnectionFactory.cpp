#include "ConnectionFactory.hpp"
#include <stdexcept>

// RequestFactory

bool ConnectionFactory::ifExists(int event_fd) {
  return connections.end() != connections.find(event_fd);
}
http::Connection &ConnectionFactory::getConnection(int event_fd) {
  if (ifExists(event_fd))
    return *connections[event_fd];
  throw std::runtime_error("Connection wasn't found");
}
void ConnectionFactory::addConnection(http::Connection *req, int event_fd) {
  if (!ifExists(event_fd))
    connections[event_fd] = req;
}
void ConnectionFactory::delConnection(int event_fd) {
  if (!ifExists(event_fd))
    return;
  delete connections[event_fd];
  connections.erase(event_fd);
}
size_t ConnectionFactory::getLen() const { return connections.size(); }

ConnectionFactory::ConnectionFactory() {}
ConnectionFactory::~ConnectionFactory() {
  for (size_t i = 0; i < this->connections.size(); ++i)
    delete this->connections[i];
}
// /RequestFactory
