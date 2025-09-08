// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_DEFAULT_SHELL_TIMEOUT_IS_CONFIGURED_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_DEFAULT_SHELL_TIMEOUT_IS_CONFIGURED_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureDefaultShellTimeoutIsConfigured(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_DEFAULT_SHELL_TIMEOUT_IS_CONFIGURED_H
