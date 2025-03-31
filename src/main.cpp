#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
// #include <iostream>

#include "hooks/ConfigHandler.hpp"
#include "hooks/HandleCGI.hpp"
// #include "hooks/HandleStatic.hpp"
// #include "hooks/ParseQuery.hpp"
#include "http/http.hpp"

// void GetCars(http::Request const &req, http::Response &res) {
//   (void)req;
//   res.setHeader("Content-Length", "10");
//   res.setStatusCode(http::OK);
//   res.setStatusMessage("OK");
//   res.setBody("0123456789");
// }

// StaticHandler staticHandler("static");
// void handleStatic(http::Request const &req, http::Response &res) {
//   staticHandler(req, res);
// }

ConfigHandler configHandler(NULL);
void handleConfig(http::Request const &req, http::Response &res) {
  configHandler(req, res);
}

int main(int ac, char **av) {
  const std::string configPath = ac > 1 ? av[1] : "src/config/default.conf";
  if (access(configPath.c_str(), F_OK) == -1)
    return perror("Can't open config file"), 1;

  srvparams params;
  server &srv = server::getInstance(params, configPath);
  server::serverInst = &srv;
  configHandler.setConfig(&srv.getConfig());
  CGI cgi(".py:.js:.sh", "/usr/bin/python3:/usr/bin/node:/usr/bin/bash");

  // srv.hook(parseQueryString);
  srv.hook(cgi.handleCgi);
  srv.hook(handleConfig);
  // srv.hook(handleStatic);
  // srv.get("/", GetCars);

  if (srv.listenAndServe() == -1)
    return perror("Error on the server: "), 1;
  server::destroyInstance();
  return 0;
}
