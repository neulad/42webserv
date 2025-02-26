#include "HandleStatic.hpp"
#include "../http/http.hpp"
#include <fstream>
#include <sys/stat.h>

std::string getMimeType(const std::string &filename) {
  static std::map<std::string, std::string> mimeTypes;
  if (mimeTypes.empty()) {
    mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".csv"] = "text/csv";

    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".bmp"] = "image/bmp";
    mimeTypes[".ico"] = "image/x-icon";
    mimeTypes[".svg"] = "image/svg+xml";

    mimeTypes[".mp3"] = "audio/mpeg";
    mimeTypes[".wav"] = "audio/wav";
    mimeTypes[".ogg"] = "audio/ogg";
    mimeTypes[".mp4"] = "video/mp4";
    mimeTypes[".webm"] = "video/webm";
    mimeTypes[".avi"] = "video/x-msvideo";
    mimeTypes[".mov"] = "video/quicktime";
    mimeTypes[".flv"] = "video/x-flv";

    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".zip"] = "application/zip";
    mimeTypes[".tar"] = "application/x-tar";
    mimeTypes[".gz"] = "application/gzip";
    mimeTypes[".rar"] = "application/vnd.rar";
    mimeTypes[".7z"] = "application/x-7z-compressed";
    mimeTypes[".iso"] = "application/x-iso9660-image";
    mimeTypes[".exe"] = "application/x-msdownload";
    mimeTypes[".msi"] = "application/x-ms-installer";
    mimeTypes[".deb"] = "application/vnd.debian.binary-package";
    mimeTypes[".rpm"] = "application/x-rpm";

    mimeTypes[".ttf"] = "font/ttf";
    mimeTypes[".otf"] = "font/otf";
    mimeTypes[".woff"] = "font/woff";
    mimeTypes[".woff2"] = "font/woff2";
  }

  // Find the last dot in the filename
  std::size_t dotPos = filename.rfind('.');
  if (dotPos != std::string::npos) {
    std::string ext = filename.substr(dotPos);
    if (mimeTypes.find(ext) != mimeTypes.end())
      return mimeTypes[ext];
  }
  return "application/octet-stream"; // Default for unknown files
}

void StaticHandler::operator()(http::Request const &req,
                               http::Response &res) const {
  std::string filePath = "." + std::string(req.getUri());
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
  res.setHeader("Content-Type", getMimeType(filePath));
  file.close();
}
