// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_PASSWORD_CHANGE_DATE_H
#define COMPLIANCEENGINE_PROCEDURES_PASSWORD_CHANGE_DATE_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct PasswordChangeDateParams
{
    /// Path to the shadow file to test against
    Optional<std::string> test_etcShadowPath = std::string("/etc/shadow");
};

Result<Status> AuditPasswordChangeDate(const PasswordChangeDateParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_PASSWORD_CHANGE_DATE_H
