#pragma once

#include <sys/epoll.h>
#include <vector>

/**
 * Many names here come from NGINX
 */
typedef struct s_srvparams {
  bool production;
  int client_header_buffer_size;
  int worker_connections;
  s_srvparams()
      : production(false), client_header_buffer_size(1024),
        worker_connections(512) {}
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
