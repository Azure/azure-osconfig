// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_PASSWORD_CHANGE_IS_IN_PAST_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_PASSWORD_CHANGE_IS_IN_PAST_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct EnsurePasswordChangeIsInPastParams
{
    /// Path to the shadow file to test against
    Optional<std::string> test_etcShadowPath = std::string("/etc/shadow");
};

Result<Status> AuditEnsurePasswordChangeIsInPast(const EnsurePasswordChangeIsInPastParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_PASSWORD_CHANGE_IS_IN_PAST_H
