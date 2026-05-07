// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_NO_SHELL_ACCOUNTS_LOCKED_H
#define COMPLIANCEENGINE_PROCEDURES_NO_SHELL_ACCOUNTS_LOCKED_H

#include <Evaluator.h>
#include <Separated.h>

namespace ComplianceEngine
{

struct NoShellAccountsLockedParams
{
    /// List of users to be excluded from check
    Optional<Separated<std::string, ','>> excludeUsers;
    /// Parse /etc/login.defs and skip users with uid below UID_MIN
    Optional<bool> skipBelowUidMin = false;
    /// Parse /etc/shells and if user does not have valid shell skip it
    Optional<bool> skipInvalidShells = false;
};

Result<Status> AuditNoShellAccountsLocked(const NoShellAccountsLockedParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_NO_SHELL_ACCOUNTS_LOCKED_H
