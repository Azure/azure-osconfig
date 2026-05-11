// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_NO_WORLD_WRITABLE_FILES_H
#define COMPLIANCEENGINE_PROCEDURES_NO_WORLD_WRITABLE_FILES_H

#include <Evaluator.h>
namespace ComplianceEngine
{

Result<Status> AuditNoWorldWritableFiles(IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_NO_WORLD_WRITABLE_FILES_H
