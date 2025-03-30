#include "Config.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
// ServerConfig default constructor
ServerConfig::ServerConfig()
    : host("0.0.0.0"), port(80), client_max_body_size(10485760) // default 10MB
{}

// Private parsing method
void Config::_parseConfig(std::ifstream &file) {
  std::string line;
  ServerConfig currentServer;
  RouteConfig currentRoute;
  std::string currentRoutePath;
  bool insideServer = false;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string key;
    iss >> key;

    if (key.empty() || key[0] == '#')
      continue; // Skip comments and empty lines

    if (key == "server") {
      if (insideServer) {
        _serverConfigs.push_back(currentServer); // Save previous server
        currentServer = ServerConfig();          // Reset for new server
      }
      insideServer = true;
    } else if (key == "listen") {
      iss >> currentServer.port;
    } else if (key == "host") {
      iss >> currentServer.host;
    } else if (key == "server_name") {
      iss >> currentServer.server_name;
    } else if (key == "error_page") {
      int code;
      std::string path;
      iss >> code >> path;
      currentServer.error_pages[code] = path;
    } else if (key == "client_max_body_size") {
      std::string size;
      iss >> size;
      currentServer.client_max_body_size = std::strtoul(size.c_str(), NULL, 10);
    } else if (key == "route") {
      if (!currentRoutePath.empty()) {
        currentServer.routes[currentRoutePath] = currentRoute;
        currentRoute = RouteConfig(); // Reset route configuration
      }
      iss >> currentRoutePath;
    } else if (key == "methods") {
      std::string method;
      while (iss >> method) {
        currentRoute.methods.push_back(method);
      }
    } else if (key == "root") {
      iss >> currentRoute.root;
    } else if (key == "index") {
      iss >> currentRoute.index;
    } else if (key == "redirect") {
      iss >> currentRoute.redirect;
    } else if (key == "upload") {
      iss >> currentRoute.upload;
    }
  }

  // Save the last route if any
  if (!currentRoutePath.empty()) {
    currentServer.routes[currentRoutePath] = currentRoute;
  }

  // Save the last server if inside one
  if (insideServer) {
    _serverConfigs.push_back(currentServer);
  }
}

// Config constructor
Config::Config(const std::string &configPath) : _configPath(configPath) {
  std::ifstream file(configPath.c_str());
  if (file.is_open()) {
    _parseConfig(file);
  } else {
    throw std::runtime_error("Failed to open configuration file.");
  }
}

Config::~Config() {
  // Nothing to free explicitly
}

// Getter to access parsed server configurations
std::vector<ServerConfig> const &Config::getServerConfigs() const {
  return _serverConfigs;
}

const std::string &Config::getConfigPath() const { return _configPath; }
