#include <string>
#include <utility>
#include <vector>

namespace http {
class request {
 private:
  std::string method;
  std::string url;
  std::string protocol;
  std::vector<std::pair<std::string, std::string> > headers;
  std::string body;

 public:
  std::string getHeader(std::string const &name) const;
  std::string getMethod();
  std::string getUrl();
  std::string getProtocol();
  std::string getBody();

  request(std::string const &raw_req);
  request(request const &req);
  request operator=(request const &req);
  ~request();
};

class response {};
};  // namespace http
