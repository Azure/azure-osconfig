// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <iostream>
#include <string>

namespace ComplianceEngine
{

AUDIT_FN(EnsureMountPointExists, "mountPoint:Mount point to check:M")
{
    auto it = args.find("mountPoint");
    if (it == args.end())
    {
        return Error("No mount point provided");
    }
    auto mountPoint = std::move(it->second);

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
        if (reportedMountPoint == mountPoint)
        {
            return indicators.Compliant("Mount point " + mountPoint + " is mounted");
        }
    }

    return indicators.NonCompliant("Mount point " + mountPoint + " is not mounted");
}

} // namespace ComplianceEngine
