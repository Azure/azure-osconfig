// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <SCE.h>

namespace ComplianceEngine
{
Result<Status> AuditSCE(const SCEParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    const auto env = params.ENVIRONMENT.ValueOr(std::string());
    return indicators.NonCompliant(std::string("SCE scripts are not supported yet (path: '") + params.scriptName + "', env: '" + env + "')");
}

Result<Status> RemediateSCE(const SCEParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    const auto env = params.ENVIRONMENT.ValueOr(std::string());
    return indicators.NonCompliant(std::string("SCE scripts are not supported yet (path: '") + params.scriptName + "', env: '" + env + "')");
}
} // namespace ComplianceEngine
