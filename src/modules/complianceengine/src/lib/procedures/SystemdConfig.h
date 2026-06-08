// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_SYSTEMD_CONFIG_H
#define COMPLIANCEENGINE_PROCEDURES_SYSTEMD_CONFIG_H

#include <Evaluator.h>
#include <Regex.h>

namespace ComplianceEngine
{

enum class SystemdConfigValueOperator
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
struct SystemdConfigValueParams
{
    /// Parameter name
    std::string parameter;

    /// Regex for the value, can be used instead of operator + value comparison
    Optional<regex> valueRegex;

    /// Operator for the value
    Optional<SystemdConfigValueOperator> op;

    /// Value to compare with
    Optional<std::string> value;

    /// Config filename
    Optional<std::string> file;

    /// Block in which the parameter is expected to be (e.g. [Unit], [Service], etc.)
    Optional<std::string> block;

    /// Directory to search for config files
    Optional<std::string> dir;

    /// If the value is not found return Compliant
    Optional<bool> passOnNotFound = false;
};

Result<Status> AuditSystemdConfigValue(const SystemdConfigValueParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_SYSTEMD_CONFIG_H
