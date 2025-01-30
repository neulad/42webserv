#pragma once

#include <string>
#include <sys/epoll.h>
#include <vector>

#define MAX_EVENTS 10

typedef struct s_srvparams {
  bool production;
  int buffer_size;
} srvparams;

class server {
private:
  int srvfd;
  int port;
  int epollfd;
  bool stop_proc;
  static std::vector<server *> servers;
  struct epoll_event events[MAX_EVENTS];
  int bindSocket();
  int addEpollEvent(int fd);
  void removeEpollEvent(int fd);
  int handleRequests();
  static void handleSignals(int signal);

public:
  class Request {
  private:
    std::string method;
    bool methodReady;
    std::string url;
    bool urlReady;
    std::string protocol;
    bool protocolReady;
    // clang-format off
  std::vector<std::pair<std::string, std::string> > headers;
    // clang-format on
    bool headersReady;
    bool bodyReady;
    // TODO: add body class and keep it's instance here

  public:
    void handleData(int fd, char *buffer, int buffer_size);
    Request();
    Request &operator=(Request const &req);
    ~Request();
  };

  server(int port, srvparams const &params);
  ~server();

  int listenAndServe();
  void stop();
  srvparams const params;
};
