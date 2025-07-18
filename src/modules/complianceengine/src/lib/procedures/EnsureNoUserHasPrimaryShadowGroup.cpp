// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <UsersIterator.h>
#include <grp.h>
#include <string>

namespace ComplianceEngine
{
AUDIT_FN(EnsureNoUserHasPrimaryShadowGroup)
{
    UNUSED(args);
    UNUSED(context);

    struct group* shadow = getgrnam("shadow");
    if (nullptr == shadow)
    {
        return Error("Group 'shadow' not found", EINVAL);
    }

    auto users = UsersRange::Make(context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& pwd : users.Value())
    {
        if (shadow->gr_gid == pwd.pw_gid)
        {
            return indicators.NonCompliant("User's '" + std::string(pwd.pw_name) + "' primary group is 'shadow'");
        }
    }

    return indicators.Compliant("No user has 'shadow' as primary group");
}

} // namespace ComplianceEngine
