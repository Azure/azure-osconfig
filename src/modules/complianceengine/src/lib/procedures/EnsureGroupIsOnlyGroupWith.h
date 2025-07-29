// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_GROUP_IS_ONLY_GROUP_WITH_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_GROUP_IS_ONLY_GROUP_WITH_H

#include <Evaluator.h>

namespace ComplianceEngine
{
// "group:A pattern or value to match group names against",
// "gid:A value to match the GID against::\\d+",
// "test_etcGroupPath:Alternative path to the /etc/group file to test against")

struct EnsureGroupIsOnlyGroupWithParams
{
    /// A pattern or value to match group names against
    std::string group;

    /// A value to match the GID against
    /// pattern: \d+
    Optional<int> gid;

    /// Alternative path to the /etc/group file to test against
    Optional<std::string> test_etcGroupPath = std::string("/etc/group");
};

Result<Status> AuditEnsureGroupIsOnlyGroupWith(const EnsureGroupIsOnlyGroupWithParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_GROUP_IS_ONLY_GROUP_WITH_H
