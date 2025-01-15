#include <sys/epoll.h>

#define MAX_EVENTS 10

class server {
 private:
  int srvfd;
  int epollfd;
  struct epoll_event events[MAX_EVENTS];
  int bindSocket();
  int addEpollEvent(int fd);
  void removeEpollEvent(int fd);
  int handleRequests();

 public:
  server();
  server(server const &srv);
  server operator=(server const &srv);
  ~server();
  int listenAndServe();
};
