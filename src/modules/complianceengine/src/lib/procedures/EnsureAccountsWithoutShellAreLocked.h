// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_ACCOUNTS_WITHOUT_SHELL_ARE_LOCKED_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_ACCOUNTS_WITHOUT_SHELL_ARE_LOCKED_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureAccountsWithoutShellAreLocked(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_ACCOUNTS_WITHOUT_SHELL_ARE_LOCKED_H
