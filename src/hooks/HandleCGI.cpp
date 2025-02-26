#include "HandleCGI.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <unistd.h>

int safeOpen(std::string const &path, int mode) {
    int output = open(path.c_str(), mode, 0644);
    if (output < 0)
        throw http::HttpError("Couldn't create a tmp file", http::InternalServerError);
    else
        return (output);
}

pid_t safeFork() {
    pid_t pid = fork();
    if (pid < 0)
        throw http::HttpError("Couldn't create a child process", http::InternalServerError);
    else
        return (pid);
}

std::string getQueryString(std::string uri) {
    ssize_t pos = uri.find("?");
    return uri.substr(pos + 1, uri.size());
}

std::string getPath(std::string uri) {
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

std::string readFileToString(const std::string& filename) {
    std::ifstream file;
    file.open(filename.c_str());
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void setHeader(std::string response, http::Response &res) {
    std::string key;
    std::string value;
    ssize_t pos = response.find(":");
    ssize_t pos2 = response.find("\n");

    if (pos <= 0)
        throw std::runtime_error("Mariusz smierdzi ze hej");
    key = response.substr(0, pos);
    value = response.substr(pos + 2, pos2);
    res.setHeader(key, value);
}

void setBody(std::string response, http::Response &res) {
    ssize_t pos = response.find("\n");
    std::string body = response.substr(pos + 1, response.size());
    res.setBody(body);
    int contentLength = response.size() - pos;
    std::stringstream ss;
    ss << contentLength;
    res.setHeader("Content-Length", ss.str());
}

void handleChild(const char *path, const char *query, bool isPost, int *pipefd) {
    if (isPost) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
    }
    if (access(path, F_OK) == 0) {
        int output = safeOpen("output.txt", O_CREAT | O_TRUNC | O_RDWR);
        setenv("QUERY_STRING", query, 1);
        const char *args[] = {"/usr/bin/python3", path, NULL};
        extern char **environ;
        dup2(output, STDOUT_FILENO);
        close(output);
        execve(args[0], (char* const*)args, environ);
    }
    throw http::HttpError("Can't access the CGI file.", http::BadRequest);
}

void handleParent(const char *query, bool isPost, int *pipefd, pid_t pid) {
    if (isPost) {
        close(pipefd[0]);
        write(pipefd[1], query, strlen(query));
        close(pipefd[1]);
    }
    waitpid(pid, NULL, 0);
}

bool isCgi(std::string path) {
    if (path.find(".py")) {
        return true;
    }
    return false;
}

void handleCgi(http::Request const &req, http::Response &res) {
    if (!isCgi(getPath(req.getUri())))
        return ;
    bool isPost = utils::cmpWebStrs(req.getMethod(), (char*)"POST");
    pid_t pid = safeFork();
    int pipefd[2];
    std::string uri = req.getUri();
    std::string queryString = getQueryString(uri);
    if (isPost)
        pipe(pipefd);
    if (pid == 0) {
        handleChild(getPath(uri).c_str(), queryString.c_str(), isPost, pipefd);
    } else {
        handleParent(queryString.c_str(), isPost, pipefd, pid);
    }
    std::string result = readFileToString("output.txt");
    setBody(result, res);
    setHeader(result, res);
    std::remove("output.txt");
}
