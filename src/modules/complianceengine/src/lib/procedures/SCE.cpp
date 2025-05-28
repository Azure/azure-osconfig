// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>

namespace ComplianceEngine
{
AUDIT_FN(SCE, "scriptName:Script path:M", "ENVIRONMENT:Environment as passed to the SCE script")
{
    UNUSED(context);
    auto it = args.find("scriptName");
    auto scriptName = it != args.end() ? std::move(it->second) : std::string();

    it = args.find("ENVIRONMENT");
    auto env = it != args.end() ? std::move(it->second) : std::string();

    return indicators.NonCompliant(std::string("SCE scripts are not supported yet (path: '") + scriptName + "', env: '" + env + "')");
}

REMEDIATE_FN(SCE, "scriptName:Script path:M", "ENVIRONMENT:Environment as passed to the SCE script")
{
    UNUSED(context);
    auto it = args.find("scriptName");
    auto scriptName = it != args.end() ? std::move(it->second) : std::string();

    it = args.find("ENVIRONMENT");
    auto env = it != args.end() ? std::move(it->second) : std::string();

    return indicators.NonCompliant(std::string("SCE scripts are not supported yet (path: '") + scriptName + "', env: '" + env + "')");
}
} // namespace ComplianceEngine
