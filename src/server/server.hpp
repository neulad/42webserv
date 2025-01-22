#include <sys/epoll.h>
#include <vector>

#define MAX_EVENTS 10

typedef struct s_srvparams {
  bool production;
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
  server(int port, srvparams const &params);
  ~server();
  int listenAndServe();
  void stop();
  srvparams const params;
};
