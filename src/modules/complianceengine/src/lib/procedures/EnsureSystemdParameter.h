// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_CONFIG_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_CONFIG_H

#include <Evaluator.h>
#include <Regex.h>

namespace ComplianceEngine
{
struct SystemdParameterParams
{
    /// Parameter name
    std::string parameter;

    /// Regex for the value
    regex valueRegex;

    /// Config filename
    Optional<std::string> file;

    /// Directory to search for config files
    Optional<std::string> dir;
};

Result<Status> AuditSystemdParameter(const SystemdParameterParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_CONFIG_H
