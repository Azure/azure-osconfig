// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <StringTools.h>
#include <algorithm>
#include <stdexcept>

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

Result<int> TryStringToInt(const std::string& str, int base)
{
    try
    {
        return std::stoi(str, nullptr, base);
    }
    catch (const std::invalid_argument&)
    {
        return Error("Invalid integer value: " + str, EINVAL);
    }
    catch (const std::out_of_range&)
    {
        return Error("Integer value out of range: " + str, ERANGE);
    }
}
} // namespace ComplianceEngine
