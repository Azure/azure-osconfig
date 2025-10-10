// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_FIREWALL_OPEN_PORTS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_FIREWALL_OPEN_PORTS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
Result<Status> AuditEnsureIptablesOpenPorts(IndicatorsTree& indicators, ContextInterface& context);
Result<Status> AuditEnsureIp6tablesOpenPorts(IndicatorsTree& indicators, ContextInterface& context);
Result<Status> AuditEnsureUfwOpenPorts(IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_FIREWALL_OPEN_PORTS_H
