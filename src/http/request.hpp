#include <ext/hash_map>
#include <string>
#include <vector>

class Request {
private:
  std::string method;
  std::string url;
  std::string protocol;
  // clang-format off
  std::vector<std::pair<std::string, std::string> > headers;
  // clang-format on
  std::string body;

public:
  Request(int fd);

  std::string getHeader(std::string const &name) const;
  // clang-format off
  const std::vector<std::pair<std::string, std::string> > &getAllHeaders() const;
  // clang-format on
  std::string getMethod() const;
  std::string getUrl() const;
  std::string getProtocol() const;
  ~Request();
};

class RequestFactory {
private:
  /**
   * We store each of the requests in the hash table
   * when the request is done with it's deleted
   */
  __gnu_cxx::hash_map<int, Request> requests;

public:
  bool ifExists(int fd);
  const Request &getRequest(int fd);
  RequestFactory();
  ~RequestFactory();
};
