// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_LOG_FILE_ACCESS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_LOG_FILE_ACCESS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct EnsureLogfileAccessParams
{
    /// Path to log directory to check, default /var/log
    Optional<std::string> path = std::string("/var/log");
};

Result<Status> AuditEnsureLogfileAccess(const EnsureLogfileAccessParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateEnsureLogfileAccess(const EnsureLogfileAccessParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_LOG_FILE_ACCESS_H
