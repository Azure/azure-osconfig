// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_PASSWD_GROUPS_EXIST_H
#define COMPLIANCEENGINE_PROCEDURES_PASSWD_GROUPS_EXIST_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditPasswdGroupsExist(IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediatePasswdGroupsExist(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_PASSWD_GROUPS_EXIST_H
