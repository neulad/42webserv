#include "./CGIUtils.hpp"

int safeOpen(std::string const &path, int mode) {
  int output = open(path.c_str(), mode, 0644);
  if (output < 0)
    throw http::HttpError("Couldn't create a tmp file",
                          http::InternalServerError);
  else
    return (output);
}

pid_t safeFork() {
  pid_t pid = fork();
  if (pid < 0)
    throw http::HttpError("Couldn't create a child process",
                          http::InternalServerError);
  else
    return (pid);
}

std::string getQueryString(std::string uri) {
  ssize_t pos = uri.find("?");
  return uri.substr(pos + 1, uri.size());
}

std::string getScriptPath(std::string uri) {
  ssize_t pos1 = uri.find("/");
  ssize_t pos2 = uri.find("?");
  std::string path;
  if (pos2 != 0) {
    path = uri.substr(pos1 + 1, pos2 - 1);
  } else {
    path = uri.substr(pos1 + 1, uri.size());
  }
  return path;
}

std::string readFileToString(const std::string &filename) {
  std::ifstream file;
  file.open(filename.c_str());
  if (!file) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void setResHeader(std::string response, http::Response &res) {
  std::string key;
  std::string value;
  ssize_t pos = response.find(":");
  ssize_t pos2 = response.find_first_of("\n");
  if (pos <= 0)
    throw std::runtime_error("Mariusz smierdzi ze hej");
  key = response.substr(0, pos);
  value = response.substr(pos + 2, pos2 - (pos + 2));
  res.setHeader(key, value);
}

void setResBody(std::string response, http::Response &res) {
  ssize_t pos = response.find_first_of("\n");
  std::string body = response.substr(pos + 1, response.size());
  res.setBody(body);
  int contentLength = body.length();
  std::stringstream ss;
  ss << contentLength;
  res.setHeader("Content-Length", ss.str());
}

bool isCgi(const std::string &path) {
  size_t pos = path.find_last_of('.');
  if (pos != std::string::npos) {
    std::string ext = path.substr(pos);
    return (ext == ".py" || ext == ".sh");
  }
  return false;
}
