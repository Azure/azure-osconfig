// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ROOT_PATH_SECURITY_H
#define COMPLIANCEENGINE_PROCEDURES_ROOT_PATH_SECURITY_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditRootPathSecurity(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ROOT_PATH_SECURITY_H
