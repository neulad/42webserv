#include "HandleStatic.hpp"
#include "../http/http.hpp"
#include "../utils/utils.hpp"
#include <fstream>
#include <iostream>
#include <sys/stat.h>

void StaticHandler::operator()(http::Request const &req,
                               http::Response &res) const {
  // Extract URI and ignore query string
  std::string uri(req.getUri().pos);
  if (req.getUri().nxtBuf)
    uri += std::string(req.getUri().nxtBuf);

  size_t queryPos = uri.find('?');
  if (queryPos != std::string::npos) {
    uri = uri.substr(0, queryPos); // Remove query string
  }

  std::string filePath = "." + uri;
  if (std::strncmp(filePath.c_str() + 2, dirPath.c_str(), dirPath.length()) !=
      0)
    return;

  std::ifstream file;
  file.open(filePath.c_str());
  if (!file.is_open())
    throw http::HttpError("The file doesn't exist", http::NotFound);
  struct stat fileStat;
  stat(filePath.c_str(), &fileStat);
  if (S_ISDIR(fileStat.st_mode))
    file.close(), throw http::HttpError("The requested path is a directory",
                                        http::Forbidden);
  res.setBodyPath(filePath);

  unsigned long fileSize = fileStat.st_size;
  std::ostringstream fileSizeString;
  fileSizeString << fileSize;

  res.setHeader("Content-Length", fileSizeString.str());
  res.setHeader("Content-Type", utils::getMimeType(filePath));
  file.close();
}
