// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_ENSURE_NO_WRITABLES
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_ENSURE_NO_WRITABLES

#include <Evaluator.h>
namespace ComplianceEngine
{

Result<Status> AuditEnsureNoWritables(IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_ENSURE_NO_WRITABLES
