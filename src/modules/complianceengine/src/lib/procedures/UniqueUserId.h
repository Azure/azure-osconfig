// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_UNIQUE_USER_ID_H
#define COMPLIANCEENGINE_PROCEDURES_UNIQUE_USER_ID_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct UniqueUserIdParams
{
    /// A value to match usernames against
    std::string username;

    /// A value to match the UID against
    /// pattern: \d+
    Optional<int> uid;

    /// A value to match the GID against
    /// pattern: \d+
    Optional<int> gid;

    /// Alternative path to the /etc/passwd file to test against
    Optional<std::string> test_etcPasswdPath = std::string("/etc/passwd");
};

Result<Status> AuditUniqueUserId(const UniqueUserIdParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_UNIQUE_USER_ID_H
