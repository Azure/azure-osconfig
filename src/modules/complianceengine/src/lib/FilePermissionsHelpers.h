// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "Indicators.h"

#include <map>
#include <string>
namespace ComplianceEngine
{
// For given filename, audit the values (owner, group, permissions, mask) given in args.
Result<Status> AuditEnsureFilePermissionsHelper(const std::string& filename, const std::map<std::string, std::string>& args, IndicatorsTree& indicators,
    ContextInterface& context);
// Remediate, as above.
Result<Status> RemediateEnsureFilePermissionsHelper(const std::string& filename, const std::map<std::string, std::string>& args,
    IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
