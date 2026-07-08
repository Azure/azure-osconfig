// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_SYSCTL_VALUE_H
#define COMPLIANCEENGINE_PROCEDURES_SYSCTL_VALUE_H

#include <Evaluator.h>
#include <Optional.h>
#include <Pattern.h>
#include <Regex.h>

namespace ComplianceEngine
{
struct SysctlValueParams
{
    /// Name of the sysctl
    /// pattern: ^([a-zA-Z0-9_]+[\.a-zA-Z0-9_-]+)$
    std::string sysctlName;

    /// Regex that the value of sysctl has to match
    Pattern value;

    /// When true, only the runtime value in /proc/sys is checked (default: false)
    /// When false (default), also validates stored sysctl configuration files
    Optional<bool> runtimeOnly = false;
};

Result<Status> AuditSysctlValue(const SysctlValueParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_SYSCTL_VALUE_H
