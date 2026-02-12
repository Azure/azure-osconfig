// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_FIREWALLD_ACTIVE_ZONE_TARGETS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_FIREWALLD_ACTIVE_ZONE_TARGETS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureFirewalldActiveZoneTargets(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_FIREWALLD_ACTIVE_ZONE_TARGETS_H
