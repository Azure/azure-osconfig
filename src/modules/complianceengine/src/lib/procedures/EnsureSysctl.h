// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSCTL_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSCTL_H

#include <Evaluator.h>
#include <Pattern.h>
#include <Regex.h>

namespace ComplianceEngine
{
struct EnsureSysctlParams
{
    /// Name of the sysctl
    /// pattern: ^([a-zA-Z0-9_]+[\.a-zA-Z0-9_-]+)$
    std::string sysctlName;

    /// Regex that the value of sysctl has to match
    Pattern value;
};

Result<Status> AuditEnsureSysctl(const EnsureSysctlParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSCTL_H
