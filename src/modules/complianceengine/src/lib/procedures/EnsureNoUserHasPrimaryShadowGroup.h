// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_USER_HAS_PRIMARY_SHADOW_GROUP_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_USER_HAS_PRIMARY_SHADOW_GROUP_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureNoUserHasPrimaryShadowGroup(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_USER_HAS_PRIMARY_SHADOW_GROUP_H
