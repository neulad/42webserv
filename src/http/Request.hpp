#include <string>
#include <vector>

class Request {
private:
  std::string method;
  bool methodReady;
  std::string url;
  bool urlReady;
  std::string protocol;
  bool protocolReady;
  // clang-format off
  std::vector<std::pair<std::string, std::string> > headers;
  // clang-format on
  bool headersReady;
  bool bodyReady;

public:
  void handleData(int fd);
  Request();
  Request &operator=(Request const &req);
  ~Request();
};