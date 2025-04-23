// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <string>

namespace compliance
{
AUDIT_FN(UfwStatus)
{
    UNUSED(args);
    auto output = context.ExecuteCommand("ufw status");
    if (!output.HasValue())
    {
        return indicators.NonCompliant("ufw not found: " + output.Error().message);
    }
    if (output.Value().find("Status: active") != std::string::npos)
    {
        return indicators.Compliant("ufw active");
    }
    else
    {
        return indicators.NonCompliant("ufw not active");
    }
}
} // namespace compliance
