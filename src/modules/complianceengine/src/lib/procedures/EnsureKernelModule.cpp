// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "KernelModuleTools.h"

#include <EnsureKernelModule.h>
#include <Evaluator.h>

namespace ComplianceEngine
{

Result<Status> AuditEnsureKernelModuleUnavailable(const EnsureKernelModuleUnavailableParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto moduleName = params.moduleName;
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
