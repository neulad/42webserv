#pragma once

#include <string>
#include <sys/epoll.h>
#include <vector>

namespace http {
class Request;
class Response;
}; // namespace http

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
  typedef void (*HookFunc)(http::Request const &req, http::Response &res);

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
  std::vector<HookFunc> hooks;

public:
  server(int port, srvparams const &params);
  ~server();

  void hook(HookFunc);
  void runHooks(http::Request &req, http::Response &res);
  int listenAndServe();
  void stop();
  srvparams const params;
};
