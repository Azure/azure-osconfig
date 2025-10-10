// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_AUDITD_RULES_CHECK_H
#define COMPLIANCEENGINE_PROCEDURES_AUDITD_RULES_CHECK_H

#include <Evaluator.h>
#include <Separated.h>

namespace ComplianceEngine
{
struct AuditAuditdRulesCheckParams
{
    /// Item being audited
    std::string searchItem;

    /// Option the checked rule line cannot include
    Optional<std::string> excludeOption;

    /// Options that should be included on the rule line, colon separated
    Separated<std::string, ':'> requiredOptions;
};

Result<Status> AuditAuditdRulesCheck(const AuditAuditdRulesCheckParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_AUDITD_RULES_CHECK_H
