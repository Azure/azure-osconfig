// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "NetworkTools.h"

#include <EnsureMTAsLocalOnly.h>
#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureMTAsLocalOnly(IndicatorsTree& indicators, ContextInterface& context)
{
    auto result = GetOpenPorts(context);
    if (!result.HasValue())
    {
        return result.Error();
    }
    const auto& openPorts = result.Value();
    for (const auto& port : openPorts)
    {
        if (!port.IsLocal() && ((port.port == 25) || (port.port == 587) || (port.port == 465)))
        {
            return indicators.NonCompliant("MTA is listening on port " + std::to_string(port.port) + " on non-local interface");
        }
    }
    return indicators.Compliant("No open MTA ports found on non-local interfaces");
}

} // namespace ComplianceEngine
