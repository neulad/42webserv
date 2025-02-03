#include "server.hpp"
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

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

/**
 * Static fields
 */
std::vector<server *> server::servers;

/**
 * Signals handling
 */
void server::handleSignals(int signal) {
  if (signal == SIGINT)
    for (size_t i = 0; i < server::servers.size(); ++i) {
      server::servers[i]->stop();
    }
}

/**
 * Requests handling
 */
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
                       this->params.worker_connections, -1);
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
      // /Check if it is a request for connection or new data

      // Check if the client has sent a FIN package
      {
        char buffer[1];
        if (recv(event_fd, buffer, 1, MSG_PEEK) <= 0) {
          close(event_fd);
          removeEpollEvent(event_fd);
          // TODO: Remove request if it was not finished from
          continue;
        }
      }
      // /Check if the client has sent a FIN package

      http::Request *req;
      if (!reqfac.ifExists(event_fd))
        reqfac.setRequest(new http::Request(this->params), event_fd);
      req = &reqfac.getRequest(event_fd);
      req->handleData(event_fd, this->params);
      std::cout << "is req set to done: " << req->header_buffer.isFull()
                << std::endl;
      //   /**
      //    * This is the part where
      //    * the request gets handled
      //    * in this part we deal with raw http
      //    * messages, and we read from the socket unless there's no more
      //    data
      //    * sent by the client, and then we put it all in one string called
      //    req
      //    */
      //   std::string raw_req;
      //   buffer[nbts] = '\0';
      //   raw_req = buffer;
      //   while ((nbts = read(event_fd, buffer, BUFFER_SIZE - 1)) > 0) {
      //     buffer[nbts] = '\0';
      //     raw_req += buffer;
      //   }
      //   /**
      //    * The request processing happens here
      //    */
      //   http::request req(raw_req);
      //   std::cout << req.getUrl() + " " + req.getMethod() + " " +
      //                    req.getProtocol()
      //             << std::endl;
      //   // clang-format off
      //   std::vector<std::pair<std::string, std::string> > allHeaders =
      //       req.getAllHeaders();
      //   // clang-format on
      //   std::cout << allHeaders[0].first + " " + allHeaders[0].second
      //             << std::endl;
      //   std::cout << allHeaders[1].first + " " + allHeaders[1].second
      //             << std::endl;
      //   std::cout << req.getBody() << std::endl;
      const char *response = "HTTP/1.1 200 OK\r\n"
                             "Content-Length: 13\r\n"
                             "Content-Type: text/plain\r\n"
                             "\r\n"
                             "Hello, world!";
      write(event_fd, response, strlen(response));
      close(event_fd);
      //   removeEpollEvent(event_fd);
      // }      }
    }
  }
  return 1;
}

/**
 * Public functions
 */
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

  /**
   * Then give it an identifier
   * When the data is sent to the machine
   * It understands which socket it should send
   * this data to through this identifier
   * The client will need to
   * know server's identifier
   * to know where to send the data
   * Treat it as IP on the Internet
   */
  if (bindSocket() == -1)
    return -1;
  // Now listen to the incoming connections
  if (listen(this->srvfd, MAX_EVENTS) == -1)
    return -1;
  if ((this->epollfd = epoll_create1(0)) == -1)
    return -1;
  /**
   * Now we give the epollo ability to track down
   * what's going on with the server. This socket
   * was made to wait for the new connection and
   * create a new socket for every separate
   * connection specifically. Epollo keeps track of
   * all of the events in the created sockets, and allows
   * us to act accordingly
   */
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

// Constructors
server::server(int port, srvparams const &params)
    : srvfd(-1), port(port), epollfd(-1), stop_proc(false), params(params) {
  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i]->port == port)
      throw std::logic_error("Ports must be different for each server");
  }
  this->events = new epoll_event[params.worker_connections];
  this->servers.push_back(this);
}
server::~server() {
  this->stop();
  delete this->events;
  for (size_t i = 0; i < this->servers.size(); ++i)
    if (this == this->servers[i])
      this->servers.erase(this->servers.begin() + i);
}
// /Constructors