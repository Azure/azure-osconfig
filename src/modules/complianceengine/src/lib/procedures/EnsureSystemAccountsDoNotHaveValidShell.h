// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEM_ACCOUNTS_DO_NOT_HAVE_VALID_SHELL_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEM_ACCOUNTS_DO_NOT_HAVE_VALID_SHELL_H

#include <BindingParsers.h>
#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureSystemAccountsDoNotHaveValidShell(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEM_ACCOUNTS_DO_NOT_HAVE_VALID_SHELL_H
