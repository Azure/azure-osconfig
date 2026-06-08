// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_COMMAND_OUTPUT_MATCH_H
#define COMPLIANCEENGINE_PROCEDURES_COMMAND_OUTPUT_MATCH_H

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

struct CommandOutputMatchParams
{
    /// Command to be executed
    std::string command;

    /// Awk transformation in the middle, optional
    Optional<std::string> awk;

    /// Regex to be matched
    std::string pattern;

    /// Type of regex, P for Perl (default) or E for Extended
    Optional<RegexType> type = RegexType::Perl;
};

Result<Status> AuditCommandOutputMatch(const CommandOutputMatchParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_COMMAND_OUTPUT_MATCH_H
