// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_NO_SHADOW_PRIMARY_GROUP_H
#define COMPLIANCEENGINE_PROCEDURES_NO_SHADOW_PRIMARY_GROUP_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditNoShadowPrimaryGroup(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_NO_SHADOW_PRIMARY_GROUP_H
