// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <map>

namespace ComplianceEngine
{

AUDIT_FN(EnsureDconf, "key:dconf key name to be checked:M", "value:Value to be verified using the operation:M",
    "operation:Type of operation to perform on variable one of equal, not equal:M:^(eq|ne)$")
{
    // auto log = context.GetLogHandle();
    auto it = args.find("key");
    if (it == args.end())
    {
        return Error("No key arg provided", EINVAL);
    }
    auto key = std::move(it->second);

    it = args.find("operation");
    if (it == args.end())
    {
        return Error("No operation arg provided", EINVAL);
    }
    auto operation = std::move(it->second);
    std::map<std::string, std::function<bool(const std::string&, const std::string&)>> ops{
        {"eq", [](const std::string& x, const std::string& y) { return x == y; }},
        {"ne", [](const std::string& x, const std::string& y) { return x != y; }}};
    auto op = ops.find(operation);

    if (op == ops.end())
    {
        return Error("Not supported operation '" + operation + "'", EINVAL);
    }
    it = args.find("value");
    if (it == args.end())
    {
        return Error("No value arg provided", EINVAL);
    }
    auto value = std::move(it->second);

    Result<std::string> dconfRead = context.ExecuteCommand("dconf read " + key);
    if (!dconfRead.HasValue())
    {
        return Error("Failed to execute dconf read " + key + " error: " + dconfRead.Error().message, dconfRead.Error().code);
    }
    auto dconfVal = dconfRead.Value();

    // remove newline character
    if (*dconfVal.rbegin() == '\n')
    {
        dconfVal.erase(dconfVal.size() - 1);
    }

    auto isCompliant = op->second(dconfVal, value);
    if (isCompliant)
    {
        return indicators.Compliant("Dconf read " + key + " " + operation + " value " + value);
    }
    else
    {
        return indicators.NonCompliant("Dconf read " + key + " " + operation + " value " + value);
    }
    return indicators.NonCompliant("Dconf read " + key + " " + operation + " value " + value + " Imposible");
}

} // namespace ComplianceEngine
