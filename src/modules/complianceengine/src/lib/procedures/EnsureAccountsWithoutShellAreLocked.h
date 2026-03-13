// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_ACCOUNTS_WITHOUT_SHELL_ARE_LOCKED_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_ACCOUNTS_WITHOUT_SHELL_ARE_LOCKED_H

#include <Evaluator.h>
#include <Separated.h>

namespace ComplianceEngine
{

struct AuditEnsureAccountsWithoutShellAreLockedParams
{
    /// List of users to be excluded from check
    Optional<Separated<std::string, ','>> excludeUsers;
    /// Parse /etc/login.defs and skip users with uid below UID_MIN
    Optional<bool> skip_below_uid_min = false;
};
Result<Status> AuditEnsureAccountsWithoutShellAreLocked(const AuditEnsureAccountsWithoutShellAreLockedParams& params, IndicatorsTree& indicators,
    ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_ACCOUNTS_WITHOUT_SHELL_ARE_LOCKED_H
