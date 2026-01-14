// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_PARAMETER_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_PARAMETER_H

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

enum class SystemdParameterExpression
{
    /// label: lt
    LessThan,

    /// label: le
    LessOrEqual,

    /// label: gt
    GreaterThan,

    /// label: ge
    GreaterOrEqual,

    /// label: eq
    Equal,
};

// The V4 version reflects the SCE v4 version of the systemd parameter checks.
// Keeping the old version as the semantics changed significantly between this
// and older versions of the scripts.
struct EnsureSystemdParameterV4Params
{
    /// Main configuration filename
    std::string file;

    /// Systemd section name
    std::string section;

    /// Systemd option name
    std::string option;

    /// Expression
    SystemdParameterExpression expression;

    /// Value to compare using provided expression
    std::string value;
};

Result<Status> AuditEnsureSystemdParameterV4(const EnsureSystemdParameterV4Params& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_PARAMETER_H
