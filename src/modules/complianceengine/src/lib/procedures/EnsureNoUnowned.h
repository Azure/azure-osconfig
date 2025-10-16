// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_UNOWNED
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_UNOWNED

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureNoUnowned(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_ROOT_PATH_H
