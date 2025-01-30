#include <string>
#include <vector>

namespace utils {
std::vector<std::string> splitBeforeNewline(std::string const &str,
                                            size_t const cursor);
bool iCompare(const std::string &str1, const std::string &str2);
} // namespace utils
