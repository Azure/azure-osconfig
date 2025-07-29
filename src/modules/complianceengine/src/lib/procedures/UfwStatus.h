// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_UFWSTATUS_H
#define COMPLIANCEENGINE_PROCEDURES_UFWSTATUS_H

#include <Evaluator.h>
#include <Regex.h>

namespace ComplianceEngine
{
struct AuditUfwStatusParams
{
    /// Regex that the status must match
    regex statusRegex;
};

Result<Status> AuditUfwStatus(const AuditUfwStatusParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_PROCEDURES_UFWSTATUS_H
