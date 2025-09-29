// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHD_OPTION_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHD_OPTION_H

#include <Evaluator.h>
#include <Separated.h>

namespace ComplianceEngine
{

struct EnsureSshdOptionParams
{
    /// Name of the SSH daemon option
    Separated<std::string, ','> options;

    /// One of Regex, list of regexes, string, integer threshold the option value is evaluated against
    std::string value;

    /// (match|not_match|lt|le|gt|ge) optional, defaults to regex::^(regex|match|not_match|lt|le|gt|ge)$
    Optional<std::string> op = std::string("regex");

    /// (regular|all_matches) optional, defaults to regular::^(regular|all_matches)
    Optional<std::string> mode = std::string("regular");
};

Result<Status> AuditEnsureSshdOption(const EnsureSshdOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHD_OPTION_H
