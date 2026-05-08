// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_DEFAULT_UMASK_H
#define COMPLIANCEENGINE_PROCEDURES_DEFAULT_UMASK_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditDefaultUmask(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_DEFAULT_UMASK_H
