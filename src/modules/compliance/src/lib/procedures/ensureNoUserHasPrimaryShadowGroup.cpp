// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace compliance
{
AUDIT_FN(EnsureNoUserHasPrimaryShadowGroup)
{
    UNUSED(args);

    struct group* shadow = getgrnam("shadow");
    if (nullptr == shadow)
    {
        return Error("Group 'shadow' not found", EINVAL);
    }

    setpwent();
    bool result = true;
    struct passwd* pwd = nullptr;
    for (errno = 0, pwd = getpwent(); nullptr != pwd; errno = 0, pwd = getpwent())
    {
        if (shadow->gr_gid == pwd->pw_gid)
        {
            context.GetLogstream() << "User's '" << pwd->pw_name << "' primary group is 'shadow'";
            result = false;
        }
    }
    int status = errno;
    endpwent();
    if (0 != errno)
    {
        return Error(std::string("getpwent failed: ") + strerror(status), status);
    }

    if (result)
    {
        context.GetLogstream() << "No user has 'shadow' as primary group";
    }
    return result;
}

REMEDIATE_FN(EnsureNoUserHasPrimaryShadowGroup)
{
    UNUSED(args);
    context.GetLogstream() << "Manual remediation is required to make sure that no user has 'shadow' as primary group ";
    return false;
}
} // namespace compliance
