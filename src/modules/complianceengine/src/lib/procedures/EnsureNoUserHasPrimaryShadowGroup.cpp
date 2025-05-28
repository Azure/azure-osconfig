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

    setpwent();
    struct passwd* pwd = nullptr;
    for (errno = 0, pwd = getpwent(); nullptr != pwd; errno = 0, pwd = getpwent())
    {
        if (shadow->gr_gid == pwd->pw_gid)
        {
            endpwent();
            return indicators.NonCompliant("User's '" + std::string(pwd->pw_name) + "' primary group is 'shadow'");
        }
    }
    int status = errno;
    endpwent();
    if (0 != errno)
    {
        return Error(std::string("getpwent failed: ") + strerror(status), status);
    }

    return indicators.Compliant("No user has 'shadow' as primary group");
}

} // namespace ComplianceEngine
