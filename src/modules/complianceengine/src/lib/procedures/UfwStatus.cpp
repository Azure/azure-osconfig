// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "UfwStatus.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <StringTools.h>

namespace ComplianceEngine
{
Result<Status> AuditUfwStatus(const AuditUfwStatusParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto output = context.ExecuteCommand("ufw status verbose");
    if (!output.HasValue())
    {
        return indicators.NonCompliant("ufw not found: " + output.Error().message);
    }

    if (regex_search(output.Value(), params.statusRegex) == false)
    {
        return indicators.NonCompliant("Searched value not found in UFW output");
    }
    else
    {
        return indicators.Compliant("Searched value found in UFW output");
    }
}
} // namespace ComplianceEngine
