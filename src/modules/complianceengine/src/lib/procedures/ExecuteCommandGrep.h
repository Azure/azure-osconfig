// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_EXECUTE_COMMAND_GREP_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_EXECUTE_COMMAND_GREP_H

#include <Evaluator.h>

namespace ComplianceEngine
{
enum class RegexType
{
    /// Perl regex (default)
    /// label: P
    Perl,

    /// Extended regex
    /// label: E
    Extended,

    /// Perl regex inverted
    /// label: Pv
    PerlInverted,

    /// Extended regex inverted
    /// label: Ev
    ExtendedInverted,
};

struct ExecuteCommandGrepParams
{
    /// Command to be executed
    std::string command;

    /// Awk transformation in the middle, optional
    Optional<std::string> awk;

    /// Regex to be matched
    std::string regex;

    /// Type of regex, P for Perl (default) or E for Extended
    Optional<RegexType> type = RegexType::Perl;
};

Result<Status> AuditExecuteCommandGrep(const ExecuteCommandGrepParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_EXECUTE_COMMAND_GREP_H
