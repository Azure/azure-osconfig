// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <FilePermissionsHelpers.h>
#include <ListValidShells.h>
#include <Result.h>
#include <UsersIterator.h>
#include <fcntl.h>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace ComplianceEngine
{
using std::ifstream;
using std::map;
using std::ostringstream;
using std::set;
using std::string;

AUDIT_FN(EnsureInteractiveUsersHomeDirectoriesAreConfigured)
{
    UNUSED(args);

    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto result = Status::Compliant;
    auto users = UsersRange::Make(context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& pwd : users.Value())
    {
        const auto shell = string(pwd.pw_shell);
        const auto it = validShells->find(shell);
        if (it == validShells->end())
        {
            OsConfigLogDebug(context.GetLogHandle(), "User '%s' has shell '%s' not listed in /etc/shells", pwd.pw_name, pwd.pw_shell);
            continue;
        }

        struct stat st;
        if (0 != stat(pwd.pw_dir, &st))
        {
            int status = errno;
            if (status == ENOENT)
            {
                OsConfigLogDebug(context.GetLogHandle(), "User '%s' has home directory '%s' which does not exist", pwd.pw_name, pwd.pw_dir);
                result = indicators.NonCompliant(std::string("User's '") + pwd.pw_name + "' home directory '" + pwd.pw_dir + "' does not exist");
                continue;
            }
            else
            {
                OsConfigLogError(context.GetLogHandle(), "Failed to stat home directory '%s' for user '%s': %s", pwd.pw_dir, pwd.pw_name, strerror(status));
                return Error(string("Failed to stat home directory: ") + strerror(status), status);
            }
        }

        const auto* group = getgrgid(pwd.pw_gid);
        if (nullptr == group)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to get group for user '%s': %s", pwd.pw_name, strerror(errno));
            return Error(string("Failed to get group for user: ") + strerror(errno), errno);
        }

        map<string, string> arguments = {{"filename", pwd.pw_dir}, {"mask", "027"}, {"owner", pwd.pw_name}, {"group", group->gr_name}};
        indicators.Push("EnsureFilePermissions");
        auto subResult = AuditEnsureFilePermissionsHelper(std::string(pwd.pw_dir), std::move(arguments), indicators, context);
        if (!subResult.HasValue())
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to check permissions for home directory '%s' for user '%s': %s", pwd.pw_dir, pwd.pw_name,
                subResult.Error().message.c_str());
            return subResult;
        }
        indicators.Back().status = subResult.Value();
        indicators.Pop();

        if (subResult.Value() == Status::NonCompliant)
        {
            OsConfigLogInfo(context.GetLogHandle(), "User '%s' has home directory '%s' with incorrect permissions", pwd.pw_name, pwd.pw_dir);
            indicators.NonCompliant(std::string("User's '") + pwd.pw_name + "' home directory '" + pwd.pw_dir + "' has incorrect permissions");
            result = Status::NonCompliant;
        }
    }
    return result;
}

REMEDIATE_FN(EnsureInteractiveUsersHomeDirectoriesAreConfigured)
{
    UNUSED(args);

    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto result = Status::Compliant;
    auto users = UsersRange::Make(context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& pwd : users.Value())
    {
        const auto shell = string(pwd.pw_shell);
        const auto it = validShells->find(shell);
        if (it == validShells->end())
        {
            OsConfigLogDebug(context.GetLogHandle(), "User '%s' has shell '%s' not in /etc/shells", pwd.pw_name, pwd.pw_shell);
            continue;
        }

        struct stat st;
        if (stat(pwd.pw_dir, &st) != 0)
        {
            int status = errno;
            OsConfigLogDebug(context.GetLogHandle(), "stat failed for home directory '%s' for user '%s': %s", pwd.pw_dir, pwd.pw_name, strerror(status));
            if (status == ENOENT)
            {
                // Home directory does not exist, so we need to create it
                if (0 != mkdir(pwd.pw_dir, 0750))
                {
                    status = errno;
                    OsConfigLogError(context.GetLogHandle(), "Failed to create home directory '%s' for user '%s': %s", pwd.pw_dir, pwd.pw_name, strerror(status));
                    return Error(string("Failed to create home directory: ") + strerror(status), status);
                }
            }
            else
            {
                OsConfigLogError(context.GetLogHandle(), "Failed to stat home directory '%s' for user '%s': %s", pwd.pw_dir, pwd.pw_name, strerror(status));
                return Error(string("Failed to stat home directory: ") + strerror(status), status);
            }
        }

        const auto* group = getgrgid(pwd.pw_gid);
        if (nullptr == group)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to get group for user '%s': %s", pwd.pw_name, strerror(errno));
            return Error(string("Failed to get group for user: ") + strerror(errno), errno);
        }

        map<string, string> arguments = {{"filename", pwd.pw_dir}, {"mask", "027"}, {"owner", pwd.pw_name}, {"group", group->gr_name}};
        indicators.Push("EnsureFilePermissions");
        auto subResult = RemediateEnsureFilePermissionsHelper(pwd.pw_dir, std::move(arguments), indicators, context);
        if (!subResult.HasValue())
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to remediate permissions for home directory '%s' for user '%s': %s", pwd.pw_dir,
                pwd.pw_name, subResult.Error().message.c_str());
            result = Status::NonCompliant;
        }
        indicators.Back().status = subResult.Value();
        indicators.Pop();
    }
    return result;
}

} // namespace ComplianceEngine
