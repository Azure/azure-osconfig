// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <BindingParsers.h>
#include <StringTools.h>
#include <cstring>

namespace ComplianceEngine
{
namespace BindingParsers
{
using std::string;

template <>
Result<string> Parse<string>(const string& input)
{
    return input;
}

template <>
Result<int> Parse<int>(const std::string& input)
{
    auto result = TryStringToInt(input);
    if (!result.HasValue())
    {
        return result.Error();
    }

    return std::move(result.Value());
}

template <>
Result<regex> Parse<regex>(const string& input)
{
    try
    {
        return regex(input);
    }
    catch (const std::exception& e)
    {
        return Error("Regular expression '" + input + "' compilation failed: " + e.what(), EINVAL);
    }
}

template <>
Result<Pattern> Parse<Pattern>(const string& input)
{
    return Pattern::Make(input);
}

template <>
Result<bool> Parse<bool>(const string& input)
{
    if (0 == strcasecmp("true", input.c_str()) || 0 == strcasecmp("1", input.c_str()) || 0 == strcasecmp("yes", input.c_str()))
        return true;
    if (0 == strcasecmp("false", input.c_str()) || 0 == strcasecmp("0", input.c_str()) || 0 == strcasecmp("no", input.c_str()))
        return false;
    return Error("Unsupported boolean value string representation: " + input, EINVAL);
}

template <>
Result<mode_t> Parse<mode_t>(const string& input)
{
    try
    {
        return static_cast<mode_t>(std::stol(input, nullptr, 8));
    }
    catch (const std::exception& e)
    {
        return Error("Failed to parse octal value '" + input + "': " + e.what(), EINVAL);
    }
}
} // namespace BindingParsers
} // namespace ComplianceEngine
