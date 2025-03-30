#pragma once

#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

struct RouteConfig {
public:
  std::vector<std::string> methods;
  std::string root;
  std::string index;
  std::string redirect;
  std::string upload;
};

struct ServerConfig {
public:
  std::string host;
  int port;
  std::string server_name;
  size_t client_max_body_size;
  std::map<std::string, RouteConfig> routes;
  std::map<int, std::string> error_pages;

  ServerConfig();
};

class Config {
private:
  const std::string _configPath;
  std::vector<ServerConfig> _serverConfigs;
  void _parseConfig(std::ifstream &file);

public:
  Config(const std::string &configPath);
  ~Config();

  std::vector<ServerConfig> const &getServerConfigs() const;
  const std::string &getConfigPath() const;
};
