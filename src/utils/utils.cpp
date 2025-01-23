#include <string>
#include <vector>

namespace utils {
bool onlySpaces(std::string const &str);
std::string toLowerCaseCopy(const std::string &str);
std::string trim(const std::string &str);
std::vector<std::string> splitByColon(const std::string &line);
std::string join(const std::vector<std::string> &tokens, char delim,
                 unsigned int start, unsigned int end);
std::vector<std::string> splitBySpace(const std::string &line);
std::string readLine(const std::string &raw_req, int &cursor);
} // namespace utils
