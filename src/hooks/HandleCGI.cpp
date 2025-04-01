#include "HandleCGI.hpp"
#include "../utils/CGIUtils.hpp"
#include "../utils/CGIProcessHandling.hpp"
#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

std::map<std::string, std::string> CGI::_interpretersMap;

CGI::CGI(const std::string &extension, const std::string &interpreter) : _extension(extension), _interpreter(interpreter)
{
  initMap();
}

void CGI::initMap()
{
  std::vector<std::string> extensions = split(this->_extension, ':');
  std::vector<std::string> interpreters = split(this->_interpreter, ':');

  if (extensions.size() != interpreters.size())
    throw http::HttpError("Wrong CGI arguments", http::FailedDependency);

  for (unsigned long i = 0; i < extensions.size(); ++i)
  {
    _interpretersMap[extensions[i]] = interpreters[i];
  }
}

void CGI::handleCgi(http::Request const &req, http::Response &res)
{
  unsetenv("CONTENT_LENGTH");
  if (!isCgi(getScriptPath(req.getUri()), _interpretersMap))
    return;
  std::cout << "Interpreter for " << req.getUri() << " is " << getInterpreter(getScriptPath(req.getUri()), _interpretersMap) << std::endl;
  bool isPost = (req.getMethod() == "POST");
  int pipefd[2];
  std::string uri = req.getUri();
  std::string queryString = isPost ? req.getBody() : getQueryString(uri);
  if (isPost)
  {
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
  if (pid != 0)
  {
    handleParent(queryString.c_str(), isPost, pipefd, pid);
  }
  else
  {
    handleChild(getScriptPath(uri).c_str(), queryString.c_str(), isPost,
                pipefd, const_cast<char *>(getInterpreter(getScriptPath(uri), _interpretersMap).c_str()));
  }
  std::string result = readFileToString("output.txt");
  setResBody(result, res);
  setResHeader(result, res);
  std::remove("output.txt");
}
