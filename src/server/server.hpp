#pragma once

#include <string>
#include <sys/epoll.h>
#include <vector>

/**
 * Many names here come from NGINX
 */
typedef struct s_srvparams {
  bool production;
  int const workerConnections;
  int const headerBufferSize;
  std::string protocol;
  s_srvparams()
      : production(false), workerConnections(512), headerBufferSize(8192),
        protocol("HTTP/1.1") {}
} srvparams;

class server {
private:
  int srvfd;
  int port;
  int epollfd;
  bool stop_proc;
  static std::vector<server *> servers;
  struct epoll_event *events;
  int bindSocket();
  int addEpollEvent(int fd);
  void removeEpollEvent(int fd);
  int handleRequests();
  static void handleSignals(int signal);

public:
  server(int port, srvparams const &params);
  ~server();

  int listenAndServe();
  void stop();
  srvparams const params;
};
