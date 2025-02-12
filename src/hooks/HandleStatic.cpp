#include "HandleStatic.hpp"
#include "../http/http.hpp"
#include <fstream>
#include <sys/stat.h>

void StaticHandler::operator()(http::Request const &req,
                               http::Response &res) const {
  std::string filePath = "." + std::string(req.getUri());
  if (std::strncmp(filePath.c_str() + 2, dirPath.c_str(), dirPath.length()) !=
      0)
    return;
  std::ifstream file;
  file.open(filePath.c_str());
  if (file.is_open()) {
    struct stat fileStat;
    stat(filePath.c_str(), &fileStat);
    if (S_ISDIR(fileStat.st_mode))
      file.close(), throw http::HttpError("The requested path is a directory",
                                          http::Forbidden);
    res.setBodyPath(filePath);
  } else
    file.close(),
        throw http::HttpError("The file doesn't exist", http::NotFound);
  file.close();
}
