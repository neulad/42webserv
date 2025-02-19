#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>

#include "hooks/HandleJson.hpp"
#include "hooks/HandleStatic.hpp"
#include "hooks/ParseQuery.hpp"
#include "http/http.hpp"

void Log(http::Request const &req, http::Response &res) {
  (void)res;
  std::cout << "Request URI: " << req.getUri() << std::endl;
}

void GetCars(http::Request const &req, http::Response &res) {
  (void)req;
  res.setHeader("Content-Length", "10");
  res.setStatusCode(http::OK);
  res.setStatusMessage("OK");
  res.setBody("0123456789");
}

StaticHandler staticHandler("static");
void handleStatic(http::Request const &req, http::Response &res) {
  staticHandler(req, res);
}

int main() {
  srvparams params;
  server srv(8080, params);
  srv.hook(Log);
  srv.hook(parseQueryString);
  srv.hook(handleStatic);
  // srv.hook(handleJson);

  srv.get("/", GetCars);
  if (srv.listenAndServe() == -1)
    return perror("Error on the server: "), 1;
  return 0;
}
