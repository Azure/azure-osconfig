// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <EnsureMountPointExists.h>
#include <Evaluator.h>
#include <Regex.h>
#include <iostream>
#include <string>

namespace ComplianceEngine
{

Result<Status> AuditEnsureMountPointExists(const EnsureMountPointExistsParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    Result<std::string> findMntOutput = context.ExecuteCommand("findmnt -knl");
    if (!findMntOutput.HasValue())
    {
        return Error("Failed to execute findmnt command");
    }

    std::istringstream findMntStream(findMntOutput.Value());
    std::string line;
    while (std::getline(findMntStream, line))
    {
        std::string reportedMountPoint = line.substr(0, line.find(" "));
        if (reportedMountPoint == params.mountPoint)
        {
            return indicators.Compliant("Mount point " + params.mountPoint + " is mounted");
        }
    }

    return indicators.NonCompliant("Mount point " + params.mountPoint + " is not mounted");
}

} // namespace ComplianceEngine
