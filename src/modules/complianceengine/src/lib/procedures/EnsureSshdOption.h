// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHD_OPTION_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHD_OPTION_H

#include <Evaluator.h>
#include <Separated.h>

namespace ComplianceEngine
{
enum class EnsureSshdOptionOperation
{
    /// label: regex
    Regex,

    /// label: match
    Match,

    /// label: not_match
    NotMatch,

    /// label: lt
    LessThan,

    /// label: le
    LessOrEqual,

    /// label: gt
    GreaterThan,

    /// label: ge
    GreaterOrEqual,
};

enum class EnsureSshdOptionMode
{
    /// label: regular
    Regular,

    /// label: all_matches
    AllMatches,
};

struct EnsureSshdOptionParams
{
    /// Name of the SSH daemon option, might be a comma-separated list
    /// pattern: ^[a-z0-9]+(,[a-z0-9]+)*$
    Separated<std::string, ','> option;

    /// One of Regex, list of regexes, string or integer threshold the option value is evaluated against
    std::string value;

    /// (regex|match|not_match|lt|le|gt|ge) optional, defaults to 'regex'
    /// pattern: ^(regex|match|not_match|lt|le|gt|ge)$
    Optional<EnsureSshdOptionOperation> op = EnsureSshdOptionOperation::Regex;

    /// Mode, one of (regular|all_matches). Optional, defaults to 'regular'
    /// pattern: ^(regular|all_matches)$
    Optional<EnsureSshdOptionMode> mode = EnsureSshdOptionMode::Regular;

    ///
    Optional<bool> readExtraConfigs = false;
};

Result<Status> AuditEnsureSshdOption(const EnsureSshdOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHD_OPTION_H
