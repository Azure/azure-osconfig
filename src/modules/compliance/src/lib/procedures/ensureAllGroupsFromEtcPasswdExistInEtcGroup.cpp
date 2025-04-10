// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <grp.h>
#include <pwd.h>
#include <set>
#include <string>

namespace compliance
{
AUDIT_FN(EnsureAllGroupsFromEtcPasswdExistInEtcGroup)
{
    UNUSED(args);
    UNUSED(context);

    struct group* grp = nullptr;
    struct passwd* pwd = nullptr;
    std::set<gid_t> etcGroupGroups;

    setgrent();
    for (errno = 0, grp = getgrent(); nullptr != grp; errno = 0, grp = getgrent())
    {
        etcGroupGroups.insert(grp->gr_gid);
    }
    int status = errno;
    endgrent();
    if (0 != status)
    {
        return Error(std::string("getgrent failed: ") + strerror(status), status);
    }

    setpwent();
    Status result = Status::Compliant;
    for (errno = 0, pwd = getpwent(); nullptr != pwd; errno = 0, pwd = getpwent())
    {
        if (etcGroupGroups.find(pwd->pw_gid) == etcGroupGroups.end())
        {
            result = indicators.NonCompliant(std::string("User's '") + std::string(pwd->pw_name) + "' group " + std::to_string(pwd->pw_gid) +
                                             " from /etc/passwd does not exist in /etc/group");
        }
    }
    status = errno;
    endpwent();
    if (0 != errno)
    {
        return Error(std::string("getpwent failed: ") + strerror(status), status);
    }

    if (result == Status::Compliant)
    {
        indicators.Compliant("All user groups from '/etc/passwd' exist in '/etc/group'");
    }
    return result;
}

REMEDIATE_FN(EnsureAllGroupsFromEtcPasswdExistInEtcGroup)
{
    auto result = AuditEnsureAllGroupsFromEtcPasswdExistInEtcGroup(args, indicators, context);
    if (result)
    {
        return indicators.Compliant("Audit passed, remediation not required");
    }

    return indicators.NonCompliant("Manual remediation is required to ensure all groups from /etc/passwd exist in /etc/group");
}
} // namespace compliance
