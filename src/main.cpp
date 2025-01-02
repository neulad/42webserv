#include "server/server.hpp"
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  server srv;
  if (srv.listenAndServe() == -1)
    return perror("Error on the server: "), 1;
  return 0;
}
