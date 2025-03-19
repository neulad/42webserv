#include "HandleCGI.hpp"
#include "../utils/CGIUtils.hpp"

// Helper function to parse shebang line
std::vector<std::string> parseShebang(const char *scriptPath) {
    std::ifstream file(scriptPath);
    if (!file) {
        throw http::HttpError("Can't open script", http::BadRequest);
    }

    std::string line;
    std::getline(file, line);

    // Validate shebang format
    if (line.substr(0, 2) != "#!") {
        throw http::HttpError("Missing shebang", http::BadRequest);
    }

    // Split shebang line into components
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream iss(line.substr(2));

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        throw http::HttpError("Empty shebang", http::BadRequest);
    }

    return tokens;
}

void handleChild(const char *path, const char *query, bool isPost, int *pipefd) {
    if (isPost) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
    }

    if (access(path, F_OK) != 0) {
        throw http::HttpError("Can't access CGI file", http::BadRequest);
    }

    try {
        std::vector<std::string> shebang = parseShebang(path);
        std::vector<const char*> argv;
        for (size_t i = 0; i < shebang.size(); ++i) {
            argv.push_back(shebang[i].c_str());
        }
        argv.push_back(path);
        argv.push_back(NULL);
        int output = safeOpen("output.txt", O_CREAT | O_TRUNC | O_RDWR);
        if (!isPost) {
            setenv("QUERY_STRING", query, 1);
        }
        dup2(output, STDOUT_FILENO);
        close(output);
        extern char **environ;
        execve(shebang[0].c_str(), const_cast<char* const*>(&argv[0]), environ);
        throw http::HttpError("execve failed", http::BadRequest);

    } catch (const std::exception& e) {
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

void handleCgi(http::Request const &req, http::Response &res) {
  unsetenv("CONTENT_LENGTH");
  if (!isCgi(getScriptPath(req.getUri())))
    return;
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
    handleChild(getScriptPath(uri).c_str(), queryString.c_str(), isPost, pipefd);
  }
  std::string result = readFileToString("output.txt");
  setResBody(result, res);
  setResHeader(result, res);
  std::remove("output.txt");
}
