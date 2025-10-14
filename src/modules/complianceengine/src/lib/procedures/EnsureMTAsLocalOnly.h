// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_MTA_LOCAL_ONLY_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_MTA_LOCAL_ONLY_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureMTAsLocalOnly(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_MTA_LOCAL_ONLY_H
