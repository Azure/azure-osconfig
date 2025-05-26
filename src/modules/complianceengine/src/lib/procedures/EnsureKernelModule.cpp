// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "EnsureKernelModule.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include "KernelModuleTools.h"

namespace ComplianceEngine
{

AUDIT_FN(EnsureKernelModuleUnavailable, "moduleName:Name of the kernel module:M")
{
    auto it = args.find("moduleName");
    if (it == args.end())
    {
        return Error("No module name provided");
    }
    auto moduleName = std::move(it->second);

    auto modulePresent = SearchFilesystemForModuleName(moduleName, context);
    if (!modulePresent.HasValue())
    {
        return Result<Status>(modulePresent.Error());
    }
    else if (modulePresent.Value() == false)
    {
        return indicators.Compliant("Module " + moduleName + " not found");
    }

    auto moduleLoaded = IsKernelModuleLoaded(moduleName, context);
    if (!moduleLoaded.HasValue())
    {
        return Result<Status>(moduleLoaded.Error());
    }
    else if (moduleLoaded.Value() == true)
    {
        return indicators.NonCompliant("Module " + moduleName + " is loaded");
    }

    return IsKernelModuleBlocked(moduleName, indicators, context);
}

} // namespace ComplianceEngine
