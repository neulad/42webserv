#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>

#include "hooks/ParseQuery.hpp"
#include "http/http.hpp"

void Log(http::Request const &req, http::Response &res) {
  (void)res;
  std::cout << "Request got" << req.getUri() << std::endl;
}

int main() {
  srvparams params;
  server srv(8080, params);
  srv.hook(Log);
  srv.hook(parseQueryString);

  if (srv.listenAndServe() == -1)
    return perror("Error on the server: "), 1;
  return 0;
}
