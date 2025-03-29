#pragma once

#include "Config.hpp"
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
  size_t clientBodySize;
  size_t clientBodyBufferSize;
  std::string protocol;
  s_srvparams()
      : production(false), workerConnections(512), bufferSize(8 * 1024),
        sendfileMaxChunk(1024 * 4), clientBodySize(1024 * 1000),
        clientBodyBufferSize(1024 * 1000), protocol("HTTP/1.1") {}
} srvparams;

typedef void (*HandleFunc)(http::Request const &req, http::Response &res);

class server {
private:
  static bool stop_proc;
  Config _config;
  std::vector<int> _serverFDs;
  static int epollfd;
  struct epoll_event *events;

  server(srvparams const &params, const std::string &configPath);
  ~server();

  int bindSocket(int srvfd, int port);
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
  // Core
  static server *serverInst;
  static server &getInstance(srvparams const &params,
                             const std::string &configPath) {
    static server instance(params, configPath);
    return instance;
  }
  static void destroyInstance() { serverInst = NULL; }

  void hook(HandleFunc);
  void get(std::string const endpoint, HandleFunc);
  void post(std::string const endpoint, HandleFunc);
  void del(std::string const endpoint, HandleFunc);
  void runHooks(http::Request &req, http::Response &res);
  int listenAndServe();
  void stop();
  srvparams const params;

  // Getters/Setters
  Config &getConfig();
};
