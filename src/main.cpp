#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>

#include "hooks/HandleCGI.hpp"
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

int main(int ac, char **av) {
  const std::string configPath = ac > 1 ? av[1] : "src/config/default.conf";

  srvparams params;
  // server srv(params, configPath);
  // server *srv = server::getInstance(params, configPath);
  server &srv = server::getInstance(params, configPath);
  server::serverInst = &srv;
  srv.hook(Log);
  srv.hook(handleCgi);
  srv.hook(parseQueryString);
  // srv.hook(handleStatic);

  srv.get("/", GetCars);
  if (srv.listenAndServe() == -1)
    return perror("Error on the server: "), 1;
  server::destroyInstance();
  return 0;
}
