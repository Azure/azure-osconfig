// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_UFW_STATUS_H
#define COMPLIANCEENGINE_PROCEDURES_UFW_STATUS_H

#include <Evaluator.h>
#include <Pattern.h>

namespace ComplianceEngine
{
struct UfwStatusParams
{
    /// Regex that the status must match
    Pattern statusRegex;
};

Result<Status> AuditUfwStatus(const UfwStatusParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_PROCEDURES_UFW_STATUS_H
