// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_NO_UNOWNED_FILES_H
#define COMPLIANCEENGINE_PROCEDURES_NO_UNOWNED_FILES_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditNoUnownedFiles(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_NO_UNOWNED_FILES_H
