// #include "../hooks/ParseQuery.hpp"
#include "server.hpp"
#include "../http/http.hpp"
#include "../utils/utils.hpp"
#include "Config.hpp"
#include "ConnectionFactory.hpp"
#include "FilefdFactory.hpp"
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
#include <vector>

// server
server *server::serverInst = NULL;
int server::epollfd = -1;
bool server::stop_proc;

void server::handleSignals(int signal) {
  if (signal == SIGINT && server::serverInst) {
    server::serverInst->stop();
  }
}

int server::bindSocket(int srvfd, int port) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(srvfd, (sockaddr *)&addr, sizeof(addr))) {
    close(srvfd);
    return -1;
  }
  return 1;
}

int server::addEpollEvent(int fd, enum EPOLL_EVENTS epollEvent) {
  struct epoll_event event;
  event.events = epollEvent;
  event.data.fd = fd;

  if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, fd, &event) == -1) {
    std::cerr << "epoll_ctl failed for fd: " << fd
              << " with error: " << strerror(errno) << std::endl;
    return -1;
  }
  return 1;
}

void server::removeEpollEvent(int fd) {
  epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, NULL);
}

void server::addEPOLLOUT(int event_fd) {
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLOUT;
  ev.data.fd = event_fd;
  if (epoll_ctl(epollfd, EPOLL_CTL_MOD, event_fd, &ev) == -1) {
    throw std::runtime_error("Couldn't add EPOLLOUT event");
  }
}

void server::removeEPOLLOUT(int event_fd) {
  struct epoll_event event;
  event.events = EPOLLIN & ~EPOLLOUT;
  event.data.fd = event_fd;
  if (epoll_ctl(epollfd, EPOLL_CTL_MOD, event_fd, &event) == -1)
    throw std::runtime_error("Couldn't remove EPOLLOUT event");
}

int server::handleRequests() {
  ConnectionFactory confac;
  FilefdFactory filefdfac(params.sendfileMaxChunk);
  std::signal(SIGINT, server::handleSignals);

  while (!stop_proc) {
    int n = epoll_wait(this->epollfd, this->events,
                       this->params.workerConnections, -1);

    if (stop_proc)
      break;
    if (n == -1)
      return -1;
    for (int i = 0; i < n; ++i) {
      int event_fd = events[i].data.fd;
      fcntl(event_fd, F_SETFL, fcntl(event_fd, F_GETFL, 0) | O_NONBLOCK);

      // Check if it is a request for connection or new data
      int clntfd = -1;
      for (size_t i = 0; i < this->_serverFDs.size(); ++i) {
        if (event_fd == this->_serverFDs[i]) {
          clntfd = accept(this->_serverFDs[i], NULL, NULL);
          if (clntfd == -1)
            continue;
          addEpollEvent(clntfd, EPOLLIN);
          break;
        }
      }
      if (clntfd != -1)
        continue;

      // Sending the file to the user
      if (events[i].events & EPOLLOUT) {
        if (n < 5)
          usleep(1);
        filefdfac.sendFdoffset(event_fd);
        if (!filefdfac.ifExists(event_fd)) {
          std::cout << "response sending is over" << std::endl;
          removeEPOLLOUT(event_fd);
        }
        continue;
      }

      // Check if the user sent FIN (final) package
      {
        char buffer[1];
        if (recv(event_fd, buffer, 1, MSG_PEEK) <= 0) {
          close(event_fd);
          removeEpollEvent(event_fd);
          confac.delConnection(event_fd);
          std::cerr << "test" << std::endl;
          continue;
        }
      }
      http::Connection *conn;
      if (!confac.ifExists(event_fd))
        confac.addConnection(new http::Connection(params), event_fd);
      conn = &confac.getConnection(event_fd);
    continue_next_request:
      http::Response res(params);
      try {
        conn->hndlIncStrm(event_fd);
        if (conn->status < http::ALL_DONE)
          continue;

        // Setting default headers
        res.setHeader("Connection", "keep-alive");

        // Running middlewares
        runHooks(conn->getReq(), res);

        // If the hooks haven't set body already find an endpoint handler
        if (!res.isBodyReady())
          routeRequest(conn->getReq(), res);
        res.end(event_fd, filefdfac);
      } catch (const http::HttpError &e) {
        res.setStatusCode(e.getStatus());
        res.setStatusMessage(utils::getHttpStatusMessage(e.getStatus()));
        res.setBody(e.what());
        res.end(event_fd, filefdfac);
        close(event_fd);
        removeEpollEvent(event_fd);
        confac.delConnection(event_fd);
        continue;
      } catch (const std::exception &e) {
        res.setStatusCode(http::InternalServerError);
        res.setStatusMessage("Internal Server Error");
        res.setBody(e.what());
        res.end(event_fd, filefdfac);
        close(event_fd);
        removeEpollEvent(event_fd);
        confac.delConnection(event_fd);
        continue;
      } catch (...) {
        res.setStatusCode(http::InternalServerError);
        res.setStatusMessage("Internal Server Error");
        res.setBody("Unknown error!");
        res.end(event_fd, filefdfac);
        close(event_fd);
        removeEpollEvent(event_fd);
        confac.delConnection(event_fd);
        continue;
      }
      if (filefdfac.ifExists(event_fd))
        addEPOLLOUT(event_fd);
      if (conn->status == http::NEXT_REQUEST)
        goto continue_next_request;
    }
  }
  return 1;
}

int server::listenAndServe() {
  const std::vector<ServerConfig> configs = _config.getServerConfigs();

  for (size_t i = 0; i < configs.size(); i++) {
    int srvfd = socket(AF_INET, SOCK_STREAM, 0);
    if (srvfd == -1) {
      return -1;
    }

    if (!this->params.production) {
      int opt = 1;
      if (setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(srvfd);
        return -1;
      }
    }
    if (bindSocket(srvfd, configs[i].port) == -1) {
      close(srvfd);
      return -1;
    }

    if (listen(srvfd, params.workerConnections) == -1) {
      close(srvfd);
      return -1;
    }

    if (addEpollEvent(srvfd, EPOLLIN) == -1) {
      close(srvfd);
      return -1;
    }

    std::cout << "Server is listening on port :" << configs[i].port
              << std::endl;
    _serverFDs.push_back(srvfd);
  }

  // Start handling requests
  if (handleRequests() == -1) {
    return -1;
  }

  // Cleanup
  // for (size_t i = 0; i < _serverFDs.size(); i++) {
  //   close(_serverFDs[i]);
  // }

  return 0;
}

void server::stop() {
  server::stop_proc = true;

  for (size_t i = 0; i < _serverFDs.size(); i++) {
    if (_serverFDs[i] != -1) {
      close(_serverFDs[i]);
      _serverFDs[i] = -1;
    }
  }

  if (epollfd != -1) {
    close(epollfd);
    epollfd = -1;
  }

  this->_serverFDs.clear();
}

void server::runHooks(http::Request &req, http::Response &res) {
  for (size_t i = 0; i < hooks.size(); ++i)
    hooks[i](req, res);
}

server::server(srvparams const &params, const std::string &configPath)
    : _config(configPath), params(params) {

  this->epollfd = epoll_create1(0);
  if (this->epollfd == -1) {
    std::cerr << "Failed to create epoll instance: " << strerror(errno)
              << std::endl;
    throw std::runtime_error("Failed to create epoll instance");
  }
  this->events = new epoll_event[params.workerConnections];

  //// PRINT CONFIGURATION VALUES ////
  // std::vector<ServerConfig> serverConfigs = _config.getServerConfigs();
  // for (size_t i = 0; i < serverConfigs.size(); ++i) {
  //   ServerConfig &server = serverConfigs[i];

  //   std::cout << "Server " << (i + 1) << ":\n";
  //   std::cout << "Host: " << server.host << "\n";
  //   std::cout << "Port: " << server.port << "\n";
  //   std::cout << "Server Name: " << server.server_name << "\n";
  //   std::cout << "Client Max Body Size: " << server.client_max_body_size
  //             << "\n";

  //   std::cout << "Error Pages:\n";
  //   std::map<int, std::string>::iterator it;
  //   for (it = server.error_pages.begin(); it != server.error_pages.end();
  //        ++it) {
  //     std::cout << "  " << it->first << ": " << it->second << "\n";
  //   }

  //   std::cout << "Routes:\n";
  //   std::map<std::string, RouteConfig>::iterator routeIt;
  //   for (routeIt = server.routes.begin(); routeIt != server.routes.end();
  //        ++routeIt) {
  //     std::cout << "  Path: " << routeIt->first << "\n";
  //     std::cout << "    Methods: ";

  //     std::vector<std::string>::iterator methodIt;
  //     for (methodIt = routeIt->second.methods.begin();
  //          methodIt != routeIt->second.methods.end(); ++methodIt) {
  //       std::cout << *methodIt << " ";
  //     }

  //     std::cout << "\n    Root: " << routeIt->second.root;
  //     std::cout << "\n    Index: " << routeIt->second.index;
  //     std::cout << "\n    Redirect: " << routeIt->second.redirect << "\n";
  //   }

  //   std::cout << "--------------------------\n";
  // }
  //// PRINT CONFIGURATION VALUES ////

  // for (size_t i = 0; i < servers.size(); ++i) {
  //   if (servers[i]->port == port)
  //     throw std::logic_error("Ports must be different for each server");
  // }

  // this->servers.push_back(this);
}
server::~server() {
  stop();
  delete[] events;
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
  std::map<std::string, HandleFunc> &methodEndpoints =
      router[std::string(req.getMethod().pos) +
             (req.getMethod().nxtBuf ? std::string(req.getMethod().nxtBuf)
                                     : std::string())];
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
// /server
