// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_CONFIG_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_CONFIG_H

#include <Evaluator.h>
#include <Regex.h>

namespace ComplianceEngine
{

enum class SystemdParameterOperator
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
struct SystemdParameterParams
{
    /// Parameter name
    std::string parameter;

    /// Regex for the value, can be used instead of operator + value comparison
    Optional<regex> valueRegex;

    /// Operator for the value
    Optional<SystemdParameterOperator> op;

    /// Value to compare with
    Optional<std::string> value;

    /// Config filename
    Optional<std::string> file;

    /// Block in which the parameter is expected to be (e.g. [Unit], [Service], etc.)
    Optional<std::string> block;

    /// Directory to search for config files
    Optional<std::string> dir;
};

Result<Status> AuditSystemdParameter(const SystemdParameterParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_CONFIG_H
