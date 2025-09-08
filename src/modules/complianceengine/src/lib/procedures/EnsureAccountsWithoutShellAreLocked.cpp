// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <EnsureAccountsWithoutShellAreLocked.h>
#include <ListValidShells.h>
#include <PasswordEntriesIterator.h>
#include <Result.h>
#include <UsersIterator.h>
#include <set>
#include <shadow.h>

namespace ComplianceEngine
{
using std::set;
using std::string;

Result<Status> AuditEnsureAccountsWithoutShellAreLocked(IndicatorsTree& indicators, ContextInterface& context)
{
    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
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
