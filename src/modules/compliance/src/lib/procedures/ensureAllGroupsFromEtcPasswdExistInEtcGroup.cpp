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
AUDIT_FN(ensureAllGroupsFromEtcPasswdExistInEtcGroup)
{
    UNUSED(args);

    struct group* grp = nullptr;
    struct passwd* pwd = nullptr;
    std::set<gid_t> etcGroupGroups;

    setgrent();
    for (errno = 0, grp = getgrent(); nullptr != grp; errno = 0, grp = getgrent())
    {
        etcGroupGroups.insert(grp->gr_gid);
    }
    if (0 != errno)
    {
        endgrent();
        return Error("getgrent failed with errno: " + std::to_string(errno));
    }
    endgrent();

    setpwent();
    bool result = true;
    for (errno = 0, pwd = getpwent(); nullptr != pwd; errno = 0, pwd = getpwent())
    {
        if (etcGroupGroups.find(pwd->pw_gid) == etcGroupGroups.end())
        {
            logstream << "User's '" << std::string(pwd->pw_name) << "' group " << pwd->pw_gid << " from /etc/passwd does not exist in /etc/group";
            result = false;
        }
    }
    if (0 != errno)
    {
        endpwent();
        return Error("getpwent failed with errno: " + std::to_string(errno));
    }
    endpwent();

    if (result)
    {
        logstream << "All user groups from '/etc/passwd' exist in '/etc/group'";
    }
    return result;
}

REMEDIATE_FN(ensureAllGroupsFromEtcPasswdExistInEtcGroup)
{
    UNUSED(args);
    logstream << "Manual remediation is required to ensure all groups from /etc/passwd exist in /etc/group";
    return false;
}
} // namespace compliance
