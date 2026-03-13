// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <EnsureAccountsWithoutShellAreLocked.h>
#include <ListValidShells.h>
#include <PasswordEntriesIterator.h>
#include <Result.h>
#include <Telemetry.h>
#include <Users.h>
#include <UsersIterator.h>
#include <set>
#include <shadow.h>

namespace ComplianceEngine
{
using std::set;
using std::string;

Result<Status> AuditEnsureAccountsWithoutShellAreLocked(const AuditEnsureAccountsWithoutShellAreLockedParams& params, IndicatorsTree& indicators,
    ContextInterface& context)
{
    Result<unsigned int> uidMin = Error("Uninitialised UID_MIN");
    if (params.skip_below_uid_min.HasValue() && params.skip_below_uid_min)
    {
        uidMin = GetUidMin(context);
    }

    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        OSConfigTelemetryStatusTrace("ListValidShells", validShells.Error().code);
        return validShells.Error();
    }

    auto passwords = PasswordEntryRange::Make(context.GetSpecialFilePath("/etc/shadow"), context.GetLogHandle());
    if (!passwords.HasValue())
    {
        return passwords.Error();
    }

    set<string> lockedUsers;
    for (const auto& item : passwords.Value())
    {
        if (0 == strlen(item.sp_pwdp))
        {
            continue;
        }

        if (item.sp_pwdp[0] == '!' || item.sp_pwdp[0] == '*')
        {
            lockedUsers.insert(item.sp_namp);
        }
    }

    auto users = UsersRange::Make(context.GetSpecialFilePath("/etc/passwd"), context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& user : users.Value())
    {
        const auto shell = string(user.pw_shell);
        const auto it = validShells->find(shell);
        if (it != validShells->end())
        {
            OsConfigLogDebug(context.GetLogHandle(), "User '%s' has a valid shell '%s'", user.pw_name, user.pw_shell);
            continue;
        }

        OsConfigLogDebug(context.GetLogHandle(), "User '%s' does not have a valid shell: '%s'", user.pw_name, user.pw_shell);
        if (0 == strcmp(user.pw_name, "root"))
        {
            continue;
        }
        bool shouldSkip = false;
        if (params.excludeUsers.HasValue())
        {
            for (const auto& excludeUser : params.excludeUsers->items)
            {
                if (0 == strcmp(user.pw_name, excludeUser.c_str()))
                {
                    OsConfigLogDebug(context.GetLogHandle(), "Skip User '%s' as it's on exclude list ", user.pw_name);
                    shouldSkip = true;
                    break;
                }
            }
        }
        if (shouldSkip)
        {
            continue;
        }
        if (params.skip_below_uid_min && uidMin.HasValue())
        {
            if (user.pw_uid < uidMin.Value())
            {
                OsConfigLogDebug(context.GetLogHandle(), "Skip User '%s' as it's id %d is lower than UID_MIN %d", user.pw_name, user.pw_uid, uidMin.Value());
                continue;
            }
        }

        if (lockedUsers.find(user.pw_name) == lockedUsers.end())
        {
            return indicators.NonCompliant(string("User ") + std::to_string(user.pw_uid) +
                                           " does not have a valid shell, but the account is not locked");
        }

        indicators.Compliant(string("User ") + std::to_string(user.pw_uid) + " does not have a valid shell, but the account is locked");
    }

    return indicators.Compliant("All non-root users without a login shell are locked");
}
} // namespace ComplianceEngine
