// #include "../hooks/ParseQuery.hpp"
#include "../http/http.hpp"
#include "../utils/utils.hpp"
#include "RequestFactory.hpp"
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Static fields
 */
std::vector<server *> server::servers;

void server::handleSignals(int signal) {
  if (signal == SIGINT)
    for (size_t i = 0; i < server::servers.size(); ++i) {
      server::servers[i]->stop();
    }
}

int server::bindSocket() {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(this->port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(this->srvfd, (sockaddr *)&addr, sizeof(addr))) {
    close(this->srvfd);
    return -1;
  }
  return 1;
}

int server::addEpollEvent(int fd) {
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = fd;
  if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, fd, &event) == -1)
    return -1;
  return 1;
}

void server::removeEpollEvent(int fd) {
  epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, NULL);
}

int server::handleRequests() {
  RequestFactory reqfac;
  std::signal(SIGINT, server::handleSignals);

  while (!stop_proc) {
    int n = epoll_wait(this->epollfd, this->events,
                       this->params.workerConnections, -1);
    if (stop_proc)
      break;
    if (n == -1)
      return -1;
    for (int i = 0; i < n; ++i) {
      int event_fd = this->events[i].data.fd;
      fcntl(event_fd, F_SETFL, fcntl(event_fd, F_GETFL, 0) | O_NONBLOCK);

      // Check if it is a request for connection or new data
      if (event_fd == this->srvfd) {
        int clntfd = accept(this->srvfd, NULL, NULL);
        if (clntfd == -1)
          continue;
        addEpollEvent(clntfd);
        continue;
      }
      {
        char buffer[1];
        if (recv(event_fd, buffer, 1, MSG_PEEK) <= 0) {
          close(event_fd);
          removeEpollEvent(event_fd);
          // TODO: Remove request if it was not finished from
          continue;
        }
      }
      http::Request *req;
      if (!reqfac.ifExists(event_fd))
        reqfac.setRequest(new http::Request(params), event_fd);
      req = &reqfac.getRequest(event_fd);
      req->handleData(event_fd);
      http::Response res(params);
      runHooks(*req, res);
      // res.setStatusCode(http::OK);
      // res.setStatusMessage("OK");
      // res.setHeader("Content-Type", "text/x-c++");
      // res.setBodyPath("./src/http/http.cpp");
      // queryStringType *quryString =
      //     res.getHookMap<queryStringType>("queryString");
      // if (quryString != NULL)
      //   std::cout << (*quryString)["hello"];
      if (!res.isBodyReady())
        routeRequest(*req, res);
      res.end(event_fd);
      close(event_fd);
      removeEpollEvent(event_fd);
      reqfac.deleteRequest(event_fd);
    }
  }
  return 1;
}

int server::listenAndServe() {
  // First create a socket for the server
  this->srvfd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->srvfd == -1)
    return -1;
  if (!this->params.production) {
    int opt = 1;
    if (setsockopt(this->srvfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0)
      return -1;
  }

  if (bindSocket() == -1)
    return -1;
  // Now listen to the incoming connections
  if (listen(this->srvfd, params.workerConnections) == -1)
    return -1;
  if ((this->epollfd = epoll_create1(0)) == -1)
    return -1;
  if (addEpollEvent(this->srvfd) == -1)
    return -1;
  std::cout << "Server is listening on port :" << this->port << std::endl;
  if (handleRequests() == -1)
    return -1;
  return 0;
}

void server::stop() {
  this->stop_proc = true;
  if (this->srvfd != -1)
    close(this->srvfd);
  if (this->epollfd != -1)
    close(this->epollfd);
}

void server::runHooks(http::Request &req, http::Response &res) {
  for (size_t i = 0; i < hooks.size(); ++i)
    hooks[i](req, res);
}

server::server(int port, srvparams const &params)
    : srvfd(-1), port(port), epollfd(-1), stop_proc(false), params(params) {
  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i]->port == port)
      throw std::logic_error("Ports must be different for each server");
  }
  this->events = new epoll_event[params.workerConnections];
  this->servers.push_back(this);
}
server::~server() {
  this->stop();
  delete this->events;
  for (size_t i = 0; i < this->servers.size(); ++i)
    if (this == this->servers[i])
      this->servers.erase(this->servers.begin() + i);
}

// Handlers
void server::hook(HandleFunc func) { hooks.push_back(func); }
void server::get(std::string endpoint, HandleFunc func) {
  router["GET"][endpoint] = func;
}
void server::post(std::string endpoint, HandleFunc func) {
  router["POST"][endpoint] = func;
}
void server::del(std::string endpoint, HandleFunc func) {
  router["DELETE"][endpoint] = func;
}

void server::routeRequest(http::Request const &req, http::Response &res) {
  std::map<std::string, HandleFunc> &methodEndpoints = router[req.getMethod()];
  HandleFunc func;
  for (std::map<std::string, HandleFunc>::iterator it = methodEndpoints.begin();
       it != methodEndpoints.end(); ++it) {
    if (utils::matchEndpoint(it->first, req.getUri())) {
      func = it->second;
      func(req, res);
      return;
    }
  }
  throw http::HttpError("The endpoint couldn't be found", http::NotFound);
}
// /Handlers
