
#include "HandleCGI.hpp"
#include <cstddef>
#include <fcntl.h>
#include <fstream>
#include <ostream>
#include <unistd.h>

char *joinStrings(char *str1, const char *str2) {
  size_t len1 = str1 ? std::strlen(str1) : 0;
  size_t len2 = str2 ? std::strlen(str2) : 0;

  char *result = new char[len1 + len2 + 1];
  if (!result)
    return NULL;
  if (str1) {
    std::strcpy(result, str1);
    delete[] str1;
  } else {
    result[0] = '\0';
  }
  if (str2)
    std::strcat(result, str2);
  return result;
}

char *readFdToString(int fd) {
  char *res = NULL;
  char buffer[1001];
  ssize_t bytes_read;

  if (fd < 0) {
      std::cerr << "Error: Invalid file descriptor!" << std::endl;
      return NULL;
  }
  while ((bytes_read = read(fd, buffer, 1000)) > 0) {
      buffer[bytes_read] = '\0';
      res = joinStrings(res, buffer);
  }
  if (bytes_read < 0) {
      perror("Error reading from file descriptor");
  }

  return res;
}

char *mergeBody(char *string, std::string path) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0)
    return NULL;
  char *content = readFdToString(fd);
  close(fd);
  return joinStrings(string, content);
}

void handleChild(char *path, bool isPost, int *pipefd) {
  if (isPost) {
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
  }
  setenv("QUERY_STRING", "name=Johnny&age=18", 1);
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

void parseHttpResponse(const char *response, std::string &key, std::string &value, std::string &body) {
    std::istringstream stream(response);
    std::string line;

    if (std::getline(stream, line) && !line.empty()) {
        size_t pos = line.find(":");
        if (pos != std::string::npos) {
            key = line.substr(0, pos); // Extract "Content-Type"
            value = line.substr(pos + 1); // Extract " text/html"
            value.erase(0, value.find_first_not_of(" ")); // Trim leading spaces
        }
    }

    std::getline(stream, body, '\0');
}

void handleCgi(http::Request const &req, http::Response &res) {
  int output = open("outputcgi.txt", O_CREAT | O_TRUNC | O_RDWR, 0644);
  int outCpy = dup(STDOUT_FILENO);
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
    char *tmp = !uri.nxtBuf ? strdup(uri.pos) : joinStrings(uri.pos, uri.nxtBuf);
    char *chuj = strchr(tmp, '?');
    *chuj = '\0';
    handleChild(tmp + 1, isPost, pipefd);
    // Move somewhere else (execve in handleChild)
    if (tmp != uri.pos)
      delete[] tmp;
  } else {
    char *queryString;
    http::webStr uri = req.getUri();
    char *tmp = !uri.nxtBuf ? uri.pos : joinStrings(uri.pos, uri.nxtBuf);
    if (!isPost) {
      queryString = strchr(tmp, '?') + 1;
    } else {
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
  dup2(outCpy, STDOUT_FILENO);
  close(outCpy);
  int test = open("outputcgi.txt", O_RDONLY, 0644);
  char *result = readFdToString(test);
  std::string key;
  std::string value;
  std::string body;
  parseHttpResponse(result, key, value, body);

  res.setHeader(key, value);
  res.setBody(body);
  close(output);
}

/*
TODO:
- make queryString available for child and parent
- set environments at the beginning
- try to experiment with piping output -> input, input -> output
- clean this shit a little
*/