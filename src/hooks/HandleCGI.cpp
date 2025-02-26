#include "HandleCGI.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <ostream>
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
    ssize_t pos2 = response.find_first_of("\n");
    if (pos <= 0)
        throw std::runtime_error("Mariusz smierdzi ze hej");
    key = response.substr(0, pos);
    value = response.substr(pos + 2, pos2 - (pos + 2));
    res.setHeader(key, value);
}

void setBody(std::string response, http::Response &res) {
    ssize_t pos = response.find_first_of("\n");
    std::string body = response.substr(pos + 1, response.size());
    res.setBody(body);
    int contentLength = body.length();
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
        if (!isPost)
            setenv("QUERY_STRING", query, 1);
        const char *args[] = {"/usr/bin/python3", path, NULL};
        extern char **environ;
        dup2(output, STDOUT_FILENO);
        close(output);
        execve(args[0], (char* const*)args, environ);
        throw http::HttpError("Can't exec the CGI file.", http::BadRequest);
    }
    throw http::HttpError("Can't access the CGI file.", http::BadRequest);
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

bool isCgi(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos != std::string::npos) {
        std::string ext = path.substr(pos);
        return (ext == ".py");
    }
    return false;
}

void handleCgi(http::Request const &req, http::Response &res) {
    if (!isCgi(getPath(req.getUri())))
        return ;
    bool isPost = utils::cmpWebStrs(req.getMethod(), (char*)"POST");
    int pipefd[2];
    std::string uri = req.getUri();
    std::string queryString = isPost ? req.getBody() : getQueryString(uri);
    if (isPost) {
        if (pipe(pipefd) == -1)
            throw http::HttpError("Pipes creation failed.", http::InternalServerError);
        std::stringstream ss;
        ss << queryString.length();
        setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
        setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
    }
    pid_t pid = safeFork();
    if (pid != 0) {
        handleParent(queryString.c_str(), isPost, pipefd, pid);
    } else {
        handleChild(getPath(uri).c_str(), queryString.c_str(), isPost, pipefd);
    }
    std::string result = readFileToString("output.txt");
    setBody(result, res);
    setHeader(result, res);
    std::remove("output.txt");
}
