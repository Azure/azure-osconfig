// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_PASSWORD_CHANGE_DATE_H
#define COMPLIANCEENGINE_PROCEDURES_PASSWORD_CHANGE_DATE_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditPasswordChangeDate(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_PASSWORD_CHANGE_DATE_H
