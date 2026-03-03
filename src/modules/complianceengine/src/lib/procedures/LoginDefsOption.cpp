// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <LoginDefsOption.h>
#include <StringTools.h>
#include <sstream>
#include <string>

using std::string;

namespace ComplianceEngine
{
namespace
{
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

Optional<string> FindLoginDefsValue(const string& fileContents, const string& optionName, OsConfigLogHandle logHandle)
{
    std::istringstream stream(fileContents);
    string line;
    Optional<string> foundValue;

    while (std::getline(stream, line))
    {
        auto trimmedLine = TrimWhiteSpaces(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#')
        {
            continue;
        }

        // Parse KEY VALUE format
        std::istringstream iss(trimmedLine);
        string key;
        string value;
        iss >> key >> value;

        if (key == optionName)
        {
            foundValue = value;
            OsConfigLogDebug(logHandle, "LoginDefsOption: found '%s' = '%s'", optionName.c_str(), value.c_str());
            // Don't break — last occurrence wins (login.defs standard behavior)
        }
    }

    return foundValue;
}
} // anonymous namespace

Result<Status> AuditLoginDefsOption(const LoginDefsOptionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    const string filePath = context.GetSpecialFilePath("/etc/login.defs");

    OsConfigLogDebug(context.GetLogHandle(), "LoginDefsOption: checking option '%s' %s '%s' in %s", params.option.c_str(),
        std::to_string(params.comparison).c_str(), params.value.c_str(), filePath.c_str());

    auto fileContents = context.GetFileContents(filePath);
    if (!fileContents.HasValue())
    {
        return fileContents.Error();
    }

    auto foundValue = FindLoginDefsValue(fileContents.Value(), params.option, context.GetLogHandle());
    if (!foundValue.HasValue())
    {
        return indicators.NonCompliant("Option '" + params.option + "' is not set in " + filePath);
    }

    // Try numeric comparison first
    auto lhsInt = TryStringToInt(foundValue.Value());
    auto rhsInt = TryStringToInt(params.value);

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
            return indicators.Compliant(params.option + " = " + foundValue.Value() + " (" + std::to_string(params.comparison) + " " + params.value +
                                        ")");
        }

        return indicators.NonCompliant(params.option + " = " + foundValue.Value() + " (expected " + std::to_string(params.comparison) + " " +
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
        return indicators.Compliant(params.option + " = " + foundValue.Value() + " (" + std::to_string(params.comparison) + " " + params.value + ")");
    }

    return indicators.NonCompliant(params.option + " = " + foundValue.Value() + " (expected " + std::to_string(params.comparison) + " " + params.value +
                                   ")");
}
} // namespace ComplianceEngine
