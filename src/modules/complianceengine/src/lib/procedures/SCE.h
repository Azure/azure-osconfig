// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_SCE_H
#define COMPLIANCEENGINE_PROCEDURES_SCE_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct SCEParams
{
    /// Script path
    std::string scriptName;

    /// Environment as passed to the SCE script
    Optional<std::string> ENVIRONMENT;
};

Result<Status> AuditSCE(const SCEParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateSCE(const SCEParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_SCE_H
