#pragma once

#include <map>
#include <string>
#include <sys/epoll.h>
#include <vector>

namespace http {
class Request;
class Response;
}; // namespace http

typedef struct s_srvparams {
  bool production;
  int workerConnections;
  int bufferSize;
  int sendfileMaxChunk;
  std::string protocol;
  s_srvparams()
      : production(false), workerConnections(512), bufferSize(8192),
        sendfileMaxChunk(1024 * 4), protocol("HTTP/1.1") {}
} srvparams;

typedef void (*HandleFunc)(http::Request const &req, http::Response &res);
class server {

private:
  int srvfd;
  int port;
  int epollfd;
  bool stop_proc;
  static std::vector<server *> servers;
  struct epoll_event *events;
  int bindSocket();
  int addEpollEvent(int fd, enum EPOLL_EVENTS epollEvent);
  void addEPOLLOUT(int event_fd);
  void removeEpollEvent(int fd);
  void removeEPOLLOUT(int event_fd);
  int handleRequests();
  static void handleSignals(int signal);
  std::vector<HandleFunc> hooks;
  // clang-format off
  std::map<std::string, std::map<std::string, HandleFunc> > router;
  // clang-format on
  void routeRequest(http::Request const &req, http::Response &res);

public:
  server(int port, srvparams const &params);
  ~server();

  void hook(HandleFunc);
  void get(std::string const endpoint, HandleFunc);
  void post(std::string const endpoint, HandleFunc);
  void del(std::string const endpoint, HandleFunc);
  void runHooks(http::Request &req, http::Response &res);
  int listenAndServe();
  void stop();
  srvparams const params;
};
