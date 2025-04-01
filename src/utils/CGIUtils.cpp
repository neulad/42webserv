#include "./CGIUtils.hpp"
#include <cstddef>

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
  std::cout << "Length: " << body.length() << std::endl;
  int contentLength = body.length();
  std::stringstream ss;
  ss << contentLength;
  res.setHeader("Content-Length", ss.str());
}

bool isExecutable(const std::string& path) {
    return access(path.c_str(), X_OK) == 0;
}

std::string getInterpreter(const std::string& path, const std::map<std::string, std::string>& extMap) {
    const char *ext = &path[path.find_last_of('.')];
    std::map<std::string, std::string>::const_iterator it = extMap.find((std::string)ext);
    if (it != extMap.end()) {
            return it->second;
        }
    return "";
}

bool isCgi(const std::string &path, const std::map<std::string, std::string>& extMap) {
  if (!isExecutable(path)) return false;
  if (getInterpreter(path, extMap).empty()) return false;
  return true;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, delimiter)) {
      tokens.push_back(token);
  }

  return tokens;
}