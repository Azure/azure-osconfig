// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_INTERACTIVE_USERS_HOME_DIRECTORIES_ARE_CONFIGURED_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_INTERACTIVE_USERS_HOME_DIRECTORIES_ARE_CONFIGURED_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureInteractiveUsersHomeDirectoriesAreConfigured(IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateEnsureInteractiveUsersHomeDirectoriesAreConfigured(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_INTERACTIVE_USERS_HOME_DIRECTORIES_ARE_CONFIGURED_H
