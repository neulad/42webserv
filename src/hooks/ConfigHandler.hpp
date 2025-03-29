#pragma once

#include "../http/http.hpp"
#include "../server/Config.hpp"

class ConfigHandler {
private:
  Config *config;

public:
  void operator()(http::Request const &req, http::Response &res) const;

  // Getters/Setters
  void setConfig(Config *config_);

  // Canonical
  ConfigHandler(Config *);
};
