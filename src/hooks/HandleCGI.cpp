#include "HandleCGI.hpp"
#include "../utils/CGIUtils.hpp"
#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <vector>

CGI::CGI(std::string extension, std::string interpreter): _extension(extension), _interpreter(interpreter) {
    std::cout << "CGI initialized successfully!" << std::endl;
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

void CGI::initMap() {
    std::vector<std::string> extensions = split(this->_extension, ':');
    std::vector<std::string> interpreters = split(this->_interpreter, ':');

    if (extensions.size() != interpreters.size())
        throw http::HttpError("Wrong CGI arguments", http::FailedDependency);

    for (size_t i = 0; i < extensions.size(); ++i) {
        this->_interpretersMap[extensions[i]] = interpreters[i];
    }
}

void handleChild(const char *path, const char *query, bool isPost,
                 int *pipefd) {
  if (isPost) {
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
  }

  if (access(path, F_OK) != 0) {
    throw http::HttpError("Can't access CGI file", http::BadRequest);
  }

  try {
    int output = safeOpen("output.txt", O_CREAT | O_TRUNC | O_RDWR);
    if (!isPost) {
      setenv("QUERY_STRING", query, 1);
    }
    dup2(output, STDOUT_FILENO);
    close(output);
    extern char **environ;
    char *argv[] = { (char *)"python3", (char *)path, NULL };
    execve("/usr/bin/python3", argv, environ);
    throw http::HttpError("execve failed", http::BadRequest);
  } catch (const std::exception &e) {
    throw http::HttpError(e.what(), http::BadRequest);
  }
}

void handleParent(const char *query, bool isPost, int *pipefd, pid_t pid) {
  if (isPost) {
    ssize_t written = write(pipefd[1], query, strlen(query));
    if (written == -1) {
      std::cerr << "Error writing to pipe: " << strerror(errno) << std::endl;
      exit(1);
    }
    close(pipefd[1]);
  }
  waitpid(pid, NULL, 0);
  close(pipefd[0]);
}

void CGI::handleCgi(http::Request const &req, http::Response &res) {
  unsetenv("CONTENT_LENGTH");
  if (!isCgi(getScriptPath(req.getUri()), _interpretersMap))
    return;
  std::cout << getInterpreter(getScriptPath(req.getUri()), _interpretersMap);
  bool isPost = (req.getMethod() == "POST");
  int pipefd[2];
  std::string uri = req.getUri();
  std::string queryString = isPost ? req.getBody() : getQueryString(uri);
  if (isPost) {
    if (pipe(pipefd) == -1)
      throw http::HttpError("Pipes creation failed.",
                            http::InternalServerError);
    std::stringstream ss;
    ss << queryString.length();
    std::string val = req.getHeader("Content-Type");
    setenv("CONTENT_TYPE", val.c_str(), 1);
    setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
  }
  pid_t pid = safeFork();
  if (pid != 0) {
    handleParent(queryString.c_str(), isPost, pipefd, pid);
  } else {
    handleChild(getScriptPath(uri).c_str(), queryString.c_str(), isPost,
                pipefd);
  }
  std::string result = readFileToString("output.txt");
  setResBody(result, res);
  setResHeader(result, res);
  std::remove("output.txt");
}
