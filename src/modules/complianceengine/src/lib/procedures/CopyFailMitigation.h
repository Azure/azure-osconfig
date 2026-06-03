// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_COPY_FAIL_MITIGATION_H
#define COMPLIANCEENGINE_PROCEDURES_COPY_FAIL_MITIGATION_H

#include <Evaluator.h>

namespace ComplianceEngine
{

struct CopyFailMitigationParams
{
    /// Pipe-separated absolute executable paths allowed to bind AF_ALG sockets. Empty denies every caller.
    Optional<std::string> allowedExecutablePaths;
};

Result<Status> AuditCopyFailMitigation(const CopyFailMitigationParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateCopyFailMitigation(const CopyFailMitigationParams& params, IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_PROCEDURES_COPY_FAIL_MITIGATION_H