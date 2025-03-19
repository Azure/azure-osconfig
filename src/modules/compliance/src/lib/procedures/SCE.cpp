// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>

namespace compliance
{
AUDIT_FN(SCE)
{
    auto it = args.find("scriptName");
    auto scriptName = it != args.end() ? std::move(it->second) : std::string();

    it = args.find("ENVIRONMENT");
    auto env = it != args.end() ? std::move(it->second) : std::string();

    logstream << "SCE scripts are not supported yet (path: '" << scriptName << "', env: '" << env << "')";
    return false;
}

REMEDIATE_FN(SCE)
{
    auto it = args.find("scriptName");
    auto scriptName = it != args.end() ? std::move(it->second) : std::string();

    it = args.find("ENVIRONMENT");
    auto env = it != args.end() ? std::move(it->second) : std::string();

    logstream << "SCE scripts are not supported yet (path: '" << scriptName << "', env: '" << env << "')";
    return false;
}
} // namespace compliance
