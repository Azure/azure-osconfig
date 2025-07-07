// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <StringTools.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <map>

namespace ComplianceEngine
{

AUDIT_FN(EnsureGsettings, "schema:Name of the gsettings schema to get:M", "key:Nam of gsettings key to get:M",
    "keyType:Type of key, possible options string,number:M:^(number|string)$",
    "operation:Type of operation to perform on variable one of eq, ne, lt, gt:M:^(eq|ne|lt|gt)",
    "value:value of operation to check acording to operation:M")
{
    auto log = context.GetLogHandle();
    auto it = args.find("schema");
    if (it == args.end())
    {
        return Error("No schema arg provided", EINVAL);
    }
    auto schema = EscapeForShell(it->second);

    it = args.find("key");
    if (it == args.end())
    {
        return Error("No key arg provided", EINVAL);
    }
    auto keyName = EscapeForShell(it->second);

    it = args.find("keyType");
    if (it == args.end())
    {
        return Error("No keyType arg provided", EINVAL);
    }
    auto keyType = std::move(it->second);

    it = args.find("operation");
    if (it == args.end())
    {
        return Error("No operation arg provided", EINVAL);
    }
    auto operation = std::move(it->second);
    auto operations = {std::string("eq"), std::string("ne"), std::string("lt"), std::string("gt")};
    bool supported = false;
    for (auto op : operations)
    {
        if (op == operation)
        {
            if (keyType != "number" && (operation == "lt" || operation == "gt"))
            {
                supported = false;
                break;
            }
            supported = true;
        }
    }
    if (!supported)
    {
        return Error("Not supported operation " + operation, EINVAL);
    }
    it = args.find("value");
    if (it == args.end())
    {
        return Error("No value arg provided", EINVAL);
    }
    auto value = std::move(it->second);

    char* endptr = nullptr;
    std::int64_t numberValue = 0;
    if (keyType == "number")
    {
        numberValue = strtol(value.c_str(), &endptr, 10);
        if ((nullptr == endptr) || ('\0' != *endptr))
        {
            OsConfigLogError(log, "Invalid keyValue value not a number: %s", value.c_str());
            return Error("Invalid argument value: not a number " + value, EINVAL);
        }
    }

    Result<std::string> gsettingsType = context.ExecuteCommand("gsettings range \"" + schema + "\" \"" + keyName + "\"");
    if (!gsettingsType.HasValue())
    {
        return Error("Failed to execute gsettings range command " + keyName + " error: " + gsettingsType.Error().message, gsettingsType.Error().code);
    }
    auto gsettingKeyType = gsettingsType.Value();

    // remove newline character
    if (*gsettingKeyType.rbegin() == '\n')
    {
        gsettingKeyType.erase(gsettingKeyType.size() - 1);
    }
    auto supportedKeyTypes = {std::string("type u"), std::string("type i"), std::string("type s")};
    bool isKeyTypeSuported = false;
    for (auto kt : supportedKeyTypes)
    {
        if (kt == gsettingKeyType)
        {
            isKeyTypeSuported = true;
            break;
        }
    }

    if (!isKeyTypeSuported)
    {
        return Error("Unsuported  gsettings key type for schema " + schema + " keyName  " + keyName + " keyType: " + gsettingKeyType, EINVAL);
    }

    bool valuePrefixedByUint32 = false;
    if (gsettingKeyType == "type u")
    {
        valuePrefixedByUint32 = true;
    }

    Result<std::string> gsettingsOutput = context.ExecuteCommand("gsettings get \"" + schema + "\" \"" + keyName + "\"");
    if (!gsettingsOutput.HasValue())
    {
        return Error("Failed to execute gsettings get command " + schema + " " + keyName + " error: " + gsettingsOutput.Error().message,
            gsettingsOutput.Error().code);
    }
    auto gsettingsValue = gsettingsOutput.Value();

    // remove newline character
    if (*gsettingsValue.rbegin() == '\n')
    {
        gsettingsValue.erase(gsettingsValue.size() - 1);
    }

    if (valuePrefixedByUint32)
    {
        auto prefix = std::string("uint32");
        auto pos = gsettingsValue.find(prefix);
        if (pos == std::string::npos)
        {
            return Error("Failed to parse gsettings get command " + schema + " " + keyName + " output: " + gsettingsValue + " expected uint32 prefix", EINVAL);
        }
        gsettingsValue.erase(pos, prefix.length() + 1);
    }
    std::int64_t gsettingsNumberValue = 0;
    if (gsettingKeyType == "type u" || gsettingKeyType == "type i")
    {
        endptr = nullptr;
        gsettingsNumberValue = strtol(gsettingsValue.c_str(), &endptr, 10);
        if ((nullptr == endptr) || ('\0' != *endptr))
        {
            OsConfigLogError(log, "Invalid keyValue value not a number: %s", value.c_str());
            return Error("Invalid operation value: not a number " + value, EINVAL);
        }
    }
    std::map<std::string, std::pair<std::function<bool(const int&, const int&)>, std::function<bool(const std::string&, const std::string&)>>> numStrComparisons{
        {"lt", std::make_pair([](const int& x, const int& y) { return x < y; }, [](const std::string&, const std::string&) { return false; })},
        {"gt", std::make_pair([](const int& x, const int& y) { return x > y; }, [](const std::string&, const std::string&) { return false; })},
        {"eq", std::make_pair([](const int& x, const int& y) { return x == y; }, [](const std::string& x, const std::string& y) { return x == y; })},
        {"ne", std::make_pair([](const int& x, const int& y) { return x != y; }, [](const std::string& x, const std::string& y) { return x != y; })}};
    auto genericOp = numStrComparisons.find(operation);
    if (genericOp == numStrComparisons.end())
    {
        return Error("Not supported operation " + operation, EINVAL);
    }
    bool isCompliant = false;
    if (keyType == "number")
    {
        auto op = genericOp->second.first;
        isCompliant = op(gsettingsNumberValue, numberValue);
    }
    else
    {
        auto quote = *gsettingsValue.begin();
        auto endQuote = *gsettingsValue.rbegin();
        if (quote != '"' && quote != '\'' && quote != endQuote)
        {
            return indicators.NonCompliant("Gsettings key " + schema + " " + keyName + " " + operation + " value " + value);
        }
        gsettingsValue = gsettingsValue.substr(1, gsettingsValue.length() - 2);
        auto op = genericOp->second.second;
        isCompliant = op(gsettingsValue, value);
    }

    if (isCompliant)
    {
        return indicators.Compliant("Gsettings key " + schema + " " + keyName + " " + operation + " value " + value);
    }
    else
    {
        return indicators.NonCompliant("Gsettings key " + schema + " " + keyName + " " + operation + " value " + value);
    }

    return indicators.NonCompliant("Not supported Gsettings key operation '" + operation + "'");
}

} // namespace ComplianceEngine
