// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Bindings.h>

namespace ComplianceEngine
{
using std::map;
using std::string;

Optional<Error> ParseValue(const map<string, string>& args, const string& key, int& output)
{
    std::cerr << "[" << __func__ << ":" << __LINE__ << "] "
              << "key: " << key << std::endl;
    auto it = args.find(key);
    if (it == args.end())
    {
        return Error("Missing required '" + key + "' parameter", EINVAL);
    }

    auto result = TryStringToInt(it->second);
    if (!result.HasValue())
    {
        return result.Error();
    }

    output = std::move(result).Value();
    return Optional<Error>();
}

Optional<Error> ParseValue(const map<string, string>& args, const string& key, string& output)
{
    std::cerr << "[" << __func__ << ":" << __LINE__ << "] "
              << "key: " << key << std::endl;
    auto it = args.find(key);
    if (it == args.end())
    {
        return Error("Missing required '" + key + "' parameter", EINVAL);
    }

    output = it->second;
    return Optional<Error>();
}

Optional<Error> ParseValue(const map<string, string>& args, const string& key, regex& output)
{
    std::cerr << "[" << __func__ << ":" << __LINE__ << "] "
              << "key: " << key << std::endl;
    const auto it = args.find(key);
    if (it == args.end())
    {
        return Error("Missing required '" + key + "' parameter", EINVAL);
    }

    try
    {
        output = regex(it->second);
    }
    catch (const std::exception& e)
    {
        return Error("Regular expression '" + it->second + "' compilation failed: " + e.what());
    }

    return Optional<Error>();
}
} // namespace ComplianceEngine
