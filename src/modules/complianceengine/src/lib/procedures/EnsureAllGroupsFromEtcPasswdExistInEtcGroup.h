// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_ALL_GROUPS_FROM_ETC_PASSWD_EXIST_IN_ETC_GROUP_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_ALL_GROUPS_FROM_ETC_PASSWD_EXIST_IN_ETC_GROUP_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureAllGroupsFromEtcPasswdExistInEtcGroup(IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateEnsureAllGroupsFromEtcPasswdExistInEtcGroup(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_ALL_GROUPS_FROM_ETC_PASSWD_EXIST_IN_ETC_GROUP_H
