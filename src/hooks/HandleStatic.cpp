#include "HandleStatic.hpp"
#include "../http/http.hpp"
#include <fstream>

void StaticHandler::operator()(http::Request const &req,
                               http::Response &res) const {
  std::string filePath = "." + std::string(req.getUri());
  if (!std::strncmp(filePath.c_str() + 1, dirPath.c_str(), dirPath.length()))
    return;
  std::ifstream file;
  file.open(filePath.c_str());
  if (file.is_open())
    res.setBodyPath(filePath);
  else
    throw http::HttpError("The file doesn't exist", http::NotFound);
  file.close();
}
