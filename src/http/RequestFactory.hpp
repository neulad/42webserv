#include "Request.hpp"
#include <ext/hash_map>

class RequestFactory {
private:
  __gnu_cxx::hash_map<int, Request> requests;

public:
  bool ifExists(int fd);
  Request &getRequest(int fd);
  void setRequest(Request const &req, int fd);
  void deleteRequest(int fd);
  RequestFactory();
  ~RequestFactory();
};
