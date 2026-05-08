// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_USER_DOT_FILE_PERMISSIONS_H
#define COMPLIANCEENGINE_PROCEDURES_USER_DOT_FILE_PERMISSIONS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditUserDotFilePermissions(IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateUserDotFilePermissions(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_USER_DOT_FILE_PERMISSIONS_H
