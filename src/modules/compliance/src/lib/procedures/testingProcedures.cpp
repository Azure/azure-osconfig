// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>

namespace compliance
{
REMEDIATE_FN(remediationFailure)
{
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "remediationFailure: " << it->second;
    }
    return false;
}

REMEDIATE_FN(remediationSuccess)
{
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "remediationSuccess: " << it->second;
    }
    return true;
}

AUDIT_FN(auditFailure)
{
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "auditFailure: " << it->second;
    }
    return false;
}

AUDIT_FN(auditSuccess)
{
    auto it = args.find("message");
    if (it != args.end())
    {
        logstream << "auditSuccess: " << it->second;
    }
    return true;
}

REMEDIATE_FN(remediationParametrized)
{
    UNUSED(logstream);
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
} // namespace compliance
