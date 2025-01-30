#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "server/server.hpp"

int main() {
  srvparams params = {false, 4000};
  server srv(8080, params);

  if (srv.listenAndServe() == -1)
    return perror("Error on the server: "), 1;
  return 0;
}
