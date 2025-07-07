#include <StringTools.h>
#include <algorithm>

namespace ComplianceEngine
{
std::string EscapeForShell(const std::string& str)
{
    std::string escapedStr;
    for (char c : str)
    {
        switch (c)
        {
            case '\\':
            case '"':
            case '`':
            case '$':
                escapedStr += '\\';
                // fall through
            default:
                escapedStr += c;
        }
    }
    return escapedStr;
}

std::string TrimWhiteSpaces(const std::string& str)
{
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    if (start < end)
    {
        return std::string(start, end);
    }
    return std::string();
}

} // namespace ComplianceEngine
