// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_WIRELESS_DISABLED_H
#define COMPLIANCEENGINE_PROCEDURES_WIRELESS_DISABLED_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct WirelessDisabledParams
{
    /// Optional path to the sysfs net class directory to test against
    Optional<std::string> test_sysfs_class_net = std::string("/sys/class/net");
};

Result<Status> AuditWirelessDisabled(const WirelessDisabledParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_WIRELESS_DISABLED_H
