// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_UNIQUE_GROUP_ID_H
#define COMPLIANCEENGINE_PROCEDURES_UNIQUE_GROUP_ID_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct UniqueGroupIdParams
{
    /// A pattern or value to match group names against
    std::string groupName;

    /// A value to match the GID against
    /// pattern: \d+
    Optional<int> gid;

    /// Alternative path to the /etc/group file to test against
    Optional<std::string> test_etcGroupPath = std::string("/etc/group");
};

Result<Status> AuditUniqueGroupId(const UniqueGroupIdParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_UNIQUE_GROUP_ID_H
