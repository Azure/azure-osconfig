// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Internal.h>
#include <Regex.h>
#include <string>
namespace ComplianceEngine
{
AUDIT_FN(UfwStatus, "statusRegex:Regex that the status must match:M")
{
    auto it = args.find("statusRegex");
    if (it == args.end())
    {
        return Error("Missing 'statusRegex' parameter", EINVAL);
    }
    auto statusRegexStr = std::move(it->second);

    regex statusRegex;
    try
    {
        statusRegex = regex(statusRegexStr);
    }
    catch (const std::exception& e)
    {
        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "regex", EINVAL);
        OsConfigLogError(context.GetLogHandle(), "Regex error: %s", e.what());
        return Error("Failed to compile regex '" + statusRegexStr + "' error: " + e.what());
    }

    auto output = context.ExecuteCommand("ufw status verbose");
    if (!output.HasValue())
    {
        return indicators.NonCompliant("ufw not found: " + output.Error().message);
    }

    if (regex_search(output.Value(), statusRegex) == false)
    {
        return indicators.NonCompliant("Searched value not found in UFW output");
    }
    else
    {
        return indicators.Compliant("Searched value found in UFW output");
    }
}
} // namespace ComplianceEngine
