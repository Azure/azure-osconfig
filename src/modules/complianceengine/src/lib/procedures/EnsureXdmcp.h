// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_XDCMP_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_XDCMP_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureXdmcp(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_XDCMP_H
