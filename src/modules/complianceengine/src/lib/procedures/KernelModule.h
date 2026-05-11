// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_KERNEL_MODULE_H
#define COMPLIANCEENGINE_PROCEDURES_KERNEL_MODULE_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct KernelModuleUnavailableParams
{
    /// Name of the kernel module
    std::string moduleName;
};

Result<Status> AuditKernelModuleUnavailable(const KernelModuleUnavailableParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_KERNEL_MODULE_H
