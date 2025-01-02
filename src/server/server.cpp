#include "server.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

int server::bindSocket() {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
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
  while (true) {
    int n = epoll_wait(this->epollfd, this->events, MAX_EVENTS, -1);
    if (n == -1)
      return -1;
    for (int i = 0; i < n; ++i) {
      int event_fd = this->events[i].data.fd;
      if (event_fd == this->srvfd) {
        int clntfd = accept(this->srvfd, NULL, NULL);
        if (clntfd == -1)
          return -1;
        addEpollEvent(clntfd);
      } else {
        char buffer[BUFFER_SIZE];
        int nbts = read(event_fd, buffer, BUFFER_SIZE - 1);
        if (nbts <= 0) {
          close(event_fd);
          removeEpollEvent(event_fd);
        } else {
          buffer[nbts] = '\0';
          std::cout << "Received: " << buffer << std::endl;
          const char *response = "HTTP/1.1 200 OK\r\n"
                                 "Content-Length: 13\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "\r\n"
                                 "Hello, world!";
          write(event_fd, response, strlen(response));
          close(event_fd);
          removeEpollEvent(event_fd);
        }
      }
    }
  }
}

int server::listenAndServe() {
  // First create a socket for the server
  this->srvfd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->srvfd == -1)
    return -1;
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
  if (handleRequests() == -1)
    return -1;
  return 0;
}

// Orthodox Canonical Form
server::server() {
  this->epollfd = -1;
  this->srvfd = -1;
}
server::server(server const &srv) {
  this->srvfd = srv.srvfd;
  this->epollfd = srv.epollfd;
}
server server::operator=(server const &srv) {
  if (&srv == this)
    return *this;
  this->srvfd = srv.srvfd;
  this->epollfd = srv.epollfd;
  return *this;
}
server::~server() {
  if (this->epollfd != -1)
    close(this->epollfd);
  if (this->srvfd != -1)
    close(this->srvfd);
}
