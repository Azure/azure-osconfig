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
    std::string option;

    /// Regex that the option value has to match
    regex value;
};

Result<Status> AuditEnsureSshdOption(const EnsureSshdOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);

struct EnsureSshdNoOptionParams
{
    /// Name of the SSH daemon options, comma separated
    Separated<std::string, ','> options;

    /// Comma separated list of regexes
    Separated<regex, ','> values;
};

Result<Status> AuditEnsureSshdNoOption(const EnsureSshdNoOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHD_OPTION_H
