#include "HandleCGI.hpp"
#include <fstream>

char *joinStrings(const char *str1, const char *str2) {
  if (!str1)
    return NULL;

  size_t len1 = std::strlen(str1);
  size_t len2 = (str2 ? std::strlen(str2) : 0);

  char *result = new char[len1 + len2 + 1];
  std::strcpy(result, str1);

  if (str2) {
    std::strcat(result, str2);
  }

  return result;
}

std::string readFdToString(int fd) {
  std::string content;
  char buffer[1024];
  ssize_t bytesRead;

  while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
    content.append(buffer, bytesRead);
  }

  return content;
}

char *mergeBody(char *string, std::string path) {
  int fd = open(path.c_str(), O_RDONLY);
  std::string content = readFdToString(fd);
  close(fd);
  return joinStrings(string, content.c_str());
}

void handleChild(char *path, bool isPost, int *pipefd) {
  if (isPost) {
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
  }
  if (access(path, F_OK) == 0) {
    // Later change python to check with extension using config
    const char *args[] = {"/usr/bin/python3", path, NULL};
    extern char **environ;
    execve(args[0], (char *const *)args, environ);
  } else {
    throw std::runtime_error("Co to sie stanelo sie?");
  }
}

void handleParent(char *query, bool isPost, int *pipefd, pid_t pid) {
  if (isPost) {
    close(pipefd[0]);
    write(pipefd[1], query, strlen(query));
    close(pipefd[1]);
  }
  waitpid(pid, NULL, 0);
}

void handleCgi(http::Request const &req, http::Response &res) {
  int output = open("tmp/outputcgi.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (output < 0)
    throw std::runtime_error("Couldn't open a file!");
  else {
    dup2(output, STDOUT_FILENO);
    close(output);
  }
  int pipefd[2];
  bool isPost = utils::cmpWebStrs(req.getMethod(), (char *)"POST");
  if (isPost) {
    pipe(pipefd);
  }
  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Failed to create a fork!");
  } else if (pid == 0) {
    http::webStr uri = req.getUri();
    char *tmp = !uri.nxtBuf ? uri.pos : joinStrings(uri.pos, uri.nxtBuf);
    handleChild(tmp, isPost, pipefd);
    if (tmp != uri.pos)
      delete[] tmp;
  } else {
    char *queryString;
    http::webStr uri = req.getUri();
    char *tmp = !uri.nxtBuf ? uri.pos : joinStrings(uri.pos, uri.nxtBuf);
    if (!isPost)
      queryString = strchr(tmp, '?') + 1;
    else {
      if (req.getBodyPath().empty()) {
        queryString = req.getBody();
      } else {
        queryString = mergeBody(req.getBody(), req.getBodyPath());
      }
    }
    handleParent(queryString, isPost, pipefd, pid);
    if (tmp != uri.pos)
      delete[] tmp;
    if (!req.getBodyPath().empty())
      delete[] queryString;
  }
  close(output);
}
