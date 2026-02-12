// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_LOGIN_DEFS_OPTION_H
#define COMPLIANCEENGINE_PROCEDURES_LOGIN_DEFS_OPTION_H

#include <EnsureShadowContains.h>
#include <Evaluator.h>

namespace ComplianceEngine
{
struct LoginDefsOptionParams
{
    /// The login.defs option name to check (e.g., PASS_MAX_DAYS, PASS_MIN_DAYS, PASS_WARN_AGE, ENCRYPT_METHOD)
    std::string option;

    /// The expected value to compare against
    std::string value;

    /// The comparison operation to apply
    /// pattern: ^(eq|ne|lt|le|gt|ge)$
    ComparisonOperation comparison = ComparisonOperation::Equal;

    /// Path to the login.defs file
    Optional<std::string> test_loginDefsPath = std::string("/etc/login.defs");
};

Result<Status> AuditLoginDefsOption(const LoginDefsOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_LOGIN_DEFS_OPTION_H
