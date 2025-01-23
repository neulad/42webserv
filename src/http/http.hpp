#include <string>
#include <utility>
#include <vector>

namespace http {
class request {
private:
  std::string method;
  std::string url;
  std::string protocol;
  // clang-format off
  std::vector<std::pair<std::string, std::string> > headers;
  // clang-format on
  std::string body;

public:
  std::string getHeader(std::string const &name) const;
  // clang-format off
  const std::vector<std::pair<std::string, std::string> > &getAllHeaders() const;
  // clang-format on
  std::string getMethod() const;
  std::string getUrl() const;
  std::string getProtocol() const;
  const std::string &getBody() const;

  request(std::string const &raw_req);
  ~request();
};
class response {};

/**
 * Utility functions
 */
bool onlySpaces(std::string const &str);
std::string toLowerCaseCopy(const std::string &str);
std::string trim(const std::string &str);
std::vector<std::string> splitByColon(const std::string &line);
std::string join(const std::vector<std::string> &tokens, char delim,
                 unsigned int start, unsigned int end);
std::vector<std::string> splitBySpace(const std::string &line);
std::string readLine(const std::string &raw_req, int &cursor);
}; // namespace http
