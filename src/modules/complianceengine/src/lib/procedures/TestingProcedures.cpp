// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <vector>

namespace ComplianceEngine
{
REMEDIATE_FN(RemediationFailure, "message:message to be logged")
{
    UNUSED(context);
    auto it = args.find("message");
    if (it != args.end())
    {
        return indicators.NonCompliant(it->second);
    }
    return Status::NonCompliant;
}

REMEDIATE_FN(RemediationSuccess, "message:message to be logged")
{
    UNUSED(context);
    auto it = args.find("message");
    if (it != args.end())
    {
        return indicators.NonCompliant(it->second);
    }
    return Status::Compliant;
}

AUDIT_FN(AuditFailure, "message:message to be logged")
{
    UNUSED(context);
    auto it = args.find("message");
    if (it != args.end())
    {
        return indicators.NonCompliant(it->second);
    }
    return Status::NonCompliant;
}

AUDIT_FN(AuditSuccess, "message:message to be logged")
{
    UNUSED(context);
    auto it = args.find("message");
    if (it != args.end())
    {
        return indicators.Compliant(it->second);
    }
    return Status::Compliant;
}

REMEDIATE_FN(RemediationParametrized, "result:Expected remediation result - success or failure:M")
{
    UNUSED(indicators);
    auto it = args.find("result");
    if (it == args.end())
    {
        return Error("Missing 'result' parameter");
    }

    OsConfigLogInfo(context.GetLogHandle(), "remediationParametrized: %s", it->second.c_str());
    if (it->second == "success")
    {
        return Status::Compliant;
    }
    else if (it->second == "failure")
    {
        return Status::NonCompliant;
    }

    return Error("Invalid 'result' parameter");
}

AUDIT_FN(AuditGetParamValues)
{
    UNUSED(context);
    const std::vector<std::string> keys = {"KEY1", "KEY2", "KEY3"};
    bool first = true;
    std::string rv;
    for (const auto& key : keys)
    {
        auto it = args.find(key);
        if (it != args.end())
        {
            if (!first)
            {
                rv += ", ";
            }
            rv += it->first + "=" + it->second;
            first = false;
        }
    }
    return indicators.Compliant(rv);
}
} // namespace ComplianceEngine
