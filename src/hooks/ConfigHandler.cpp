#include "./ConfigHandler.hpp"
#include "../utils/utils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// Type definitions
typedef std::map<std::string, RouteConfig>::iterator routeIter;

// Helper functions
bool removeGeckoWrapper(const std::string &filepath) {
  // Step 1: Open the file for reading in binary mode
  std::ifstream inFile(filepath.c_str(), std::ios::in | std::ios::binary);
  if (!inFile) {
    std::cerr << "Error: Could not open file for reading: " << filepath
              << std::endl;
    return false;
  }

  // Step 2: Read the entire file into a string buffer
  inFile.seekg(0, std::ios::end);
  std::streampos fileSize = inFile.tellg();
  inFile.seekg(0, std::ios::beg);

  std::string buffer(fileSize, '\0');
  inFile.read(&buffer[0], fileSize);
  inFile.close();

  // Step 3: Extract the boundary string from the first line
  std::string::size_type firstNewline = buffer.find("\r\n");
  if (firstNewline == std::string::npos) {
    std::cerr << "Error: Could not find boundary in " << filepath << std::endl;
    return false;
  }
  std::string boundary = buffer.substr(
      0,
      firstNewline); // e.g.,
                     // ------geckoformboundary3d21d1afa7a4260e20e30ae247812df9
  std::string boundaryStart = boundary; // Start boundary
  std::string boundaryEnd =
      boundary +
      "--"; // Closing boundary, e.g.,
            // ------geckoformboundary3d21d1afa7a4260e20e30ae247812df9--

  // Step 4: Find the start and end of the actual content
  std::string contentTypeMarker = "Content-Type:";
  std::string doubleNewline = "\r\n\r\n"; // Separator after headers

  // Find the start of the content (after headers)
  std::string::size_type contentTypePos = buffer.find(contentTypeMarker);
  if (contentTypePos == std::string::npos) {
    std::cerr << "Error: Could not find Content-Type in " << filepath
              << std::endl;
    return false;
  }
  std::string::size_type headerEnd = buffer.find(doubleNewline, contentTypePos);
  if (headerEnd == std::string::npos) {
    std::cerr << "Error: Could not find content start in " << filepath
              << std::endl;
    return false;
  }
  headerEnd += doubleNewline.length(); // Move past the "\r\n\r\n"

  // Find the end of the content (before the closing boundary)
  std::string::size_type contentEnd = buffer.find(boundaryEnd, headerEnd);
  if (contentEnd == std::string::npos) {
    std::cerr << "Error: Could not find content end in " << filepath
              << std::endl;
    return false;
  }

  // Extract the content between headerEnd and contentEnd
  std::string content = buffer.substr(headerEnd, contentEnd - headerEnd);

  // Step 5: Open the file for writing in binary mode (overwrite)
  std::ofstream outFile(filepath.c_str(),
                        std::ios::out | std::ios::binary | std::ios::trunc);
  if (!outFile) {
    std::cerr << "Error: Could not open file for writing: " << filepath
              << std::endl;
    return false;
  }

  // Step 6: Write the cleaned content back to the file
  outFile.write(content.c_str(), content.size());
  outFile.close();

  return true;
}

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

std::string listDirectoryAsLinks(const std::string &directoryPath,
                                 std::string currentUrl) {
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

    htmlList << "<li><a href=\"" << currentUrl + "/" + filename << "\">"
             << filename << "</a></li>" << std::endl;
  }
  htmlList << "</ul>\n";

  closedir(dir);
  return htmlList.str();
}

void rtrimCharacters(std::string &str, const std::string &charsToRemove) {
  size_t endPos = str.find_last_not_of(charsToRemove);
  if (str.size() == 1)
    return;
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
          continue;
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
        std::ofstream file(filePath.c_str(),
                           std::ios::out | std::ios::trunc | std::ios::binary);
        if (!file) {
          std::cerr << strerror(errno) << std::endl;
          throw http::HttpError("Couldn't upload the file",
                                http::InternalServerError);
        }
        file.write(req.getBody(), req.getBodyLength());
        if (!req.getBodyPath().empty()) {
          std::ifstream bodyFile(req.getBodyPath().c_str(),
                                 std::ios::in | std::ios::binary);
          file << bodyFile.rdbuf();
          bodyFile.close();
        }
        file.close();
        if (!removeGeckoWrapper(filePath)) {
          throw http::HttpError("Couldn't remove the wrapper",
                                http::InternalServerError);
        }
        res.setStatusCode(http::Created);
        res.setStatusMessage(utils::getHttpStatusMessage(http::Created));
        res.setHeader("Content-Length", "9");
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Uploaded!");
      } else if (matchRouteUrl == reqUrl && !foundMatch->second.index.empty()) {
        checkAndSetFilePath(req, res,
                            matchRouteRoot + "/" + foundMatch->second.index);
      } else if (matchRouteUrl == reqUrl) {
        std::string dirListHtml = listDirectoryAsLinks(matchRouteRoot, reqUrl);

        res.setStatusCode(http::OK);
        res.setStatusMessage(utils::getHttpStatusMessage(http::OK));
        res.setHeader("Content-Length", utils::intToString(dirListHtml.size()));
        res.setHeader("Content-Type", "text/html");
        res.setBody(dirListHtml);
      } else {
        std::string redirectUrl =
            foundMatch->second.root + "/" +
            std::string(req.getUri()).substr(foundMatch->first.length());
        if (req.getMethod() == "DELETE") {
          remove(redirectUrl.c_str());
          res.setStatusCode(http::NoContent);
          res.setStatusMessage(utils::getHttpStatusMessage(http::NoContent));
          res.setHeader("Content-Length", "7");
          res.setHeader("Content-Type", "text/plain");
          res.setBody("Deleted");

          return;
        }
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
