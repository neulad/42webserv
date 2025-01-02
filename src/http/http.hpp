#include <string>
#include <vector>

class http {
private:
  std::vector<std::string> headers;
  int status_code;
  std::string body;

public:
  http();
  http(http const &inst);
  http operator=(http const &inst);
  ~http();
};
