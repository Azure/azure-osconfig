// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UfwStatus.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <StringTools.h>
#include <sstream>

namespace ComplianceEngine
{
Result<Status> AuditUfwStatus(const UfwStatusParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    const auto* cmd = "ufw status verbose";
    auto output = context.ExecuteCommand(cmd);
    if (!output.HasValue())
    {
        return indicators.NonCompliant("ufw not found: " + output.Error().message);
    }

    OsConfigLogDebug(context.GetLogHandle(), "Command '%s' output:\n%s", cmd, output.Value().c_str());
    std::istringstream outputStream(output.Value());
    std::string line;
    bool matched = false;
    while (std::getline(outputStream, line))
    {
        if (regex_search(line, params.statusRegex.GetRegex()))
        {
            matched = true;
            break;
        }
    }

    if (!matched)
    {
        OsConfigLogInfo(context.GetLogHandle(), "Pattern '%s' did not match the output of '%s' command", params.statusRegex.GetPattern().c_str(), cmd);
        return indicators.NonCompliant("Searched value not found in UFW output");
    }
    else
    {
        OsConfigLogInfo(context.GetLogHandle(), "Pattern '%s' matched the output of '%s' command", params.statusRegex.GetPattern().c_str(), cmd);
        return indicators.Compliant("Searched value found in UFW output");
    }
}
} // namespace ComplianceEngine
