// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>

namespace compliance
{
REMEDIATE_FN(remediationFailure)
{
    UNUSED(args);
    UNUSED(logstream);
    return false;
}

REMEDIATE_FN(remediationSuccess)
{
    UNUSED(args);
    UNUSED(logstream);
    return true;
}

AUDIT_FN(auditFailure)
{
    UNUSED(args);
    UNUSED(logstream);
    return false;
}

AUDIT_FN(auditSuccess)
{
    UNUSED(args);
    UNUSED(logstream);
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
