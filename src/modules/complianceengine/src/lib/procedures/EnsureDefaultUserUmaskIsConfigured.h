// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_USER_UMASK_IS_CONFIGURED_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_USER_UMASK_IS_CONFIGURED_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureDefaultUserUmaskIsConfigured(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_USER_UMASK_IS_CONFIGURED_H
