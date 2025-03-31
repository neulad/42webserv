#include "./ConfigHandler.hpp"
#include "../utils/utils.hpp"
#include <algorithm>
#include <cstddef>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <vector>

// Type definitions
typedef std::map<std::string, RouteConfig>::iterator routeIter;

// Helper functions
void checkAndSetFilePath(http::Request const &req, http::Response &res,
                         std::string const &redirectUrl) {
  (void)req;
  std::ifstream file;
  file.open(redirectUrl.c_str());
  if (!file.is_open())
    throw http::HttpError("The file doesn't exist", http::NotFound);
  struct stat fileStat;
  stat(redirectUrl.c_str(), &fileStat);
  if (S_ISDIR(fileStat.st_mode))
    file.close(), throw http::HttpError("The requested path is a directory",
                                        http::Forbidden);
  res.setBodyPath(redirectUrl);

  unsigned long fileSize = fileStat.st_size;
  std::ostringstream fileSizeString;
  fileSizeString << fileSize;

  res.setHeader("Content-Length", fileSizeString.str());
  res.setHeader("Content-Type", utils::getMimeType(redirectUrl));
  file.close();
}

std::string listDirectoryAsLinks(const std::string &directoryPath) {
  DIR *dir = opendir(directoryPath.c_str());
  if (!dir) {
    throw http::HttpError("The directory was not found", http::NotFound);
  }

  struct dirent *entry;
  std::ostringstream htmlList;

  htmlList << "<ul>\n";
  while ((entry = readdir(dir)) != NULL) {
    std::string filename = entry->d_name;

    // Skip "." and ".." special entries
    if (filename == "." || filename == "..") {
      continue;
    }

    htmlList << "<li><a href=\"" << filename << "\">" << filename << "</a></li>"
             << std::endl;
  }
  htmlList << "</ul>\n";

  closedir(dir);
  return htmlList.str();
}

void rtrimCharacters(std::string &str, const std::string &charsToRemove) {
  size_t endPos = str.find_last_not_of(charsToRemove);
  if (endPos != std::string::npos) {
    str.erase(endPos + 1); // Trim the right side
  } else {
    str.clear(); // If all characters are to be trimmed, return an empty string
  }
}

// Core
void ConfigHandler::operator()(http::Request const &req,
                               http::Response &res) const {
  if (this->config == NULL)
    return;
  if (res.isBodyReady())
    return;

  std::vector<ServerConfig> configs = this->config->getServerConfigs();
  std::string reqUrl = std::string(req.getUri());
  rtrimCharacters(reqUrl, "/");

  for (size_t i = 0; i < configs.size(); ++i) {
    ServerConfig cnfg = configs[i];

    if (res.getPort() != cnfg.port)
      continue;
    try {
      routeIter route_iter = cnfg.routes.begin();
      routeIter route_end = cnfg.routes.end();
      routeIter foundMatch = cnfg.routes.end();

      for (; route_iter != route_end; ++route_iter) {
        std::vector<std::string> methodsAllowed = route_iter->second.methods;
        std::vector<std::string>::iterator methodExists =
            std::find(methodsAllowed.begin(), methodsAllowed.end(),
                      std::string(req.getMethod()));

        if (methodExists == methodsAllowed.end())
          return;
        if (utils::startsWith(route_iter->first, req.getUri()))
          foundMatch = route_iter;
      }

      if (foundMatch == route_end)
        return;
      std::string matchRouteUrl = foundMatch->first;
      std::string matchRouteRoot = foundMatch->second.root;

      if (!foundMatch->second.redirect.empty()) {
        res.setStatusCode(http::MovedPermanently);
        res.setStatusMessage(
            utils::getHttpStatusMessage(http::MovedPermanently));
        res.setHeader("Location", foundMatch->second.redirect);
        res.setHeader("Content-Length", "14");
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Redirecting...");
      } else if (matchRouteUrl == reqUrl &&
                 !foundMatch->second.upload.empty() &&
                 req.getMethod() == "POST" &&
                 req.getHeader("X-File-Name").size() != 0) {
        std::string fileName = req.getHeader("X-File-Name");
        std::string const filePath = foundMatch->second.upload + "/" + fileName;

        std::ofstream file(filePath.c_str(), std::ios::out | std::ios::trunc);
        if (!file) {
          throw http::HttpError("Couldn't upload the file",
                                http::InternalServerError);
        }
        file << req.getBody();
        if (!req.getBodyPath().empty()) {
          std::ofstream bodyFile(filePath.c_str(),
                                 std::ios::out | std::ios::trunc);
          file << bodyFile;
          bodyFile.close();
        }
        file.close();
        res.setStatusCode(http::Created);
        res.setStatusMessage(utils::getHttpStatusMessage(http::Created));
        res.setHeader("Content-Length", "9");
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Uploaded!");
      } else if (matchRouteUrl == reqUrl && !foundMatch->second.index.empty()) {
        checkAndSetFilePath(req, res,
                            matchRouteRoot + "/" + foundMatch->second.index);
      } else if (matchRouteUrl == reqUrl) {
        std::string dirListHtml = listDirectoryAsLinks(matchRouteRoot);

        res.setStatusCode(http::OK);
        res.setStatusMessage(utils::getHttpStatusMessage(http::OK));
        res.setHeader("Content-Length", utils::intToString(dirListHtml.size()));
        res.setHeader("Content-Type", "text/html");
        res.setBody(dirListHtml);
      } else {
        std::string redirectUrl =
            foundMatch->second.root + "/" +
            std::string(req.getUri()).substr(foundMatch->first.length());
        checkAndSetFilePath(req, res, redirectUrl);
      }
    } catch (http::HttpError const &err) {
      int errCode = err.getStatus();
      std::map<int, std::string> &errPages = cnfg.error_pages;

      std::map<int, std::string>::iterator errPagesIter = errPages.begin();
      std::map<int, std::string>::iterator errPagesEnd = errPages.end();

      for (; errPagesIter != errPagesEnd; ++errPagesIter) {
        if (errPagesIter->first == errCode) {
          checkAndSetFilePath(req, res, errPagesIter->second);
          return;
        }
      }
    }
  }
}

// Getters/Setters
void ConfigHandler::setConfig(Config *config_) { this->config = config_; }

// Canonical
ConfigHandler::ConfigHandler(Config *config_) : config(config_) {}
