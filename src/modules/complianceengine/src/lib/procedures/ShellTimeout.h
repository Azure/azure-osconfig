// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_SHELL_TIMEOUT_H
#define COMPLIANCEENGINE_PROCEDURES_SHELL_TIMEOUT_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditShellTimeout(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_PROCEDURES_SHELL_TIMEOUT_H
