// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <vector>

namespace compliance
{
REMEDIATE_FN(RemediationFailure, "message:message to be logged")
{
    UNUSED(log);
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "remediationFailure: " << it->second;
    }
    return false;
}

REMEDIATE_FN(RemediationSuccess, "message:message to be logged")
{
    UNUSED(log);
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "remediationSuccess: " << it->second;
    }
    return true;
}

AUDIT_FN(AuditFailure, "message:message to be logged")
{
    UNUSED(log);
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "auditFailure: " << it->second;
    }
    return false;
}

AUDIT_FN(AuditSuccess, "message:message to be logged")
{
    UNUSED(log);
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "auditSuccess: " << it->second;
    }
    return true;
}

REMEDIATE_FN(RemediationParametrized, "result:Expected remediation result - success or failure:M")
{
    UNUSED(logstream);
    UNUSED(log);
    auto it = args.find("result");
    if (it == args.end())
    {
        return Error("Missing 'result' parameter");
    }

    if (it->second == "success")
    {
        return true;
    }
    else if (it->second == "failure")
    {
        return false;
    }

    return Error("Invalid 'result' parameter");
}

AUDIT_FN(auditGetParamValues)
{
    UNUSED(log);
    const std::vector<std::string> keys = {"KEY1", "KEY2", "KEY3"};
    bool first = true;
    for (const auto& key : keys)
    {
        auto it = args.find(key);
        if (it != args.end())
        {
            if (!first)
            {
                logstream << ", ";
            }
            logstream << it->first << "=" << it->second;
            first = false;
        }
    }

    return true;
}
} // namespace compliance
