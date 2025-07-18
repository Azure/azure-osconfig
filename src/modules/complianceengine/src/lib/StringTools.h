#include <Result.h>
#include <string>

namespace ComplianceEngine
{

std::string EscapeForShell(const std::string& str);
std::string TrimWhiteSpaces(const std::string& str);
Result<int> TryStringToInt(const std::string& str);
} // namespace ComplianceEngine
