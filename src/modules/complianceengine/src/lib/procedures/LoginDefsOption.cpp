// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <LoginDefsOption.h>
#include <fstream>
#include <sstream>
#include <string>

using std::string;

namespace ComplianceEngine
{
namespace
{
Result<int> ParseInt(const string& value)
{
    try
    {
        return std::stoi(value);
    }
    catch (const std::invalid_argument&)
    {
        return Error("Invalid integer value: '" + value + "'", EINVAL);
    }
    catch (const std::out_of_range&)
    {
        return Error("Integer value out of range: '" + value + "'", ERANGE);
    }
}

Result<bool> NumericComparison(int lhs, int rhs, ComparisonOperation operation)
{
    switch (operation)
    {
        case ComparisonOperation::Equal:
            return lhs == rhs;
        case ComparisonOperation::NotEqual:
            return lhs != rhs;
        case ComparisonOperation::LessThan:
            return lhs < rhs;
        case ComparisonOperation::LessOrEqual:
            return lhs <= rhs;
        case ComparisonOperation::GreaterThan:
            return lhs > rhs;
        case ComparisonOperation::GreaterOrEqual:
            return lhs >= rhs;
        default:
            break;
    }

    return Error("Unsupported comparison operation for numeric value", EINVAL);
}

Result<bool> StringComparison(const string& lhs, const string& rhs, ComparisonOperation operation)
{
    switch (operation)
    {
        case ComparisonOperation::Equal:
            return lhs == rhs;
        case ComparisonOperation::NotEqual:
            return lhs != rhs;
        default:
            break;
    }

    return Error("Unsupported comparison operation for string value (only eq and ne are supported)", EINVAL);
}

static const char* ComparisonName(ComparisonOperation op)
{
    switch (op)
    {
        case ComparisonOperation::Equal:
            return "eq";
        case ComparisonOperation::NotEqual:
            return "ne";
        case ComparisonOperation::LessThan:
            return "lt";
        case ComparisonOperation::LessOrEqual:
            return "le";
        case ComparisonOperation::GreaterThan:
            return "gt";
        case ComparisonOperation::GreaterOrEqual:
            return "ge";
        default:
            return "unknown";
    }
}

Result<string> FindLoginDefsValue(const string& filePath, const string& optionName, OsConfigLogHandle logHandle)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        return Error("Failed to open login.defs file: " + filePath, errno);
    }

    string line;
    string foundValue;
    bool found = false;

    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        size_t firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace == string::npos)
        {
            continue;
        }
        if (line[firstNonSpace] == '#')
        {
            continue;
        }

        // Parse KEY VALUE format
        std::istringstream iss(line.substr(firstNonSpace));
        string key;
        string value;
        iss >> key >> value;

        if (key == optionName)
        {
            foundValue = value;
            found = true;
            OsConfigLogDebug(logHandle, "LoginDefsOption: found '%s' = '%s' in %s", optionName.c_str(), value.c_str(), filePath.c_str());
            // Don't break — last occurrence wins (login.defs standard behavior)
        }
    }

    if (!found)
    {
        return Error("Option '" + optionName + "' not found in " + filePath, ENOENT);
    }

    return foundValue;
}
} // anonymous namespace

Result<Status> AuditLoginDefsOption(const LoginDefsOptionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.test_loginDefsPath.HasValue());

    const string& filePath = params.test_loginDefsPath.Value();

    OsConfigLogInfo(context.GetLogHandle(), "LoginDefsOption: checking option '%s' %s '%s' in %s", params.option.c_str(),
        ComparisonName(params.comparison), params.value.c_str(), filePath.c_str());

    auto foundValue = FindLoginDefsValue(filePath, params.option, context.GetLogHandle());
    if (!foundValue.HasValue())
    {
        return indicators.NonCompliant("Option '" + params.option + "' is not set in " + filePath);
    }

    // Try numeric comparison first
    auto lhsInt = ParseInt(foundValue.Value());
    auto rhsInt = ParseInt(params.value);

    if (lhsInt.HasValue() && rhsInt.HasValue())
    {
        // Both values are numeric — use numeric comparison
        auto result = NumericComparison(lhsInt.Value(), rhsInt.Value(), params.comparison);
        if (!result.HasValue())
        {
            return result.Error();
        }

        if (result.Value())
        {
            return indicators.Compliant(params.option + " = " + foundValue.Value() + " (" + ComparisonName(params.comparison) + " " + params.value +
                                        ")");
        }

        return indicators.NonCompliant(params.option + " = " + foundValue.Value() + " (expected " + ComparisonName(params.comparison) + " " +
                                       params.value + ")");
    }

    // Fall back to string comparison
    auto result = StringComparison(foundValue.Value(), params.value, params.comparison);
    if (!result.HasValue())
    {
        return result.Error();
    }

    if (result.Value())
    {
        return indicators.Compliant(params.option + " = " + foundValue.Value() + " (" + ComparisonName(params.comparison) + " " + params.value + ")");
    }

    return indicators.NonCompliant(params.option + " = " + foundValue.Value() + " (expected " + ComparisonName(params.comparison) + " " + params.value +
                                   ")");
}
} // namespace ComplianceEngine
