// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <EnsureFilePermissions.h>
#include <EnsureInteractiveUsersHomeDirectoriesAreConfigured.h>
#include <Evaluator.h>
#include <ListValidShells.h>
#include <Result.h>
#include <Telemetry.h>
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

Result<Status> AuditEnsureInteractiveUsersHomeDirectoriesAreConfigured(IndicatorsTree& indicators, ContextInterface& context)
{
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

        auto pwdPattern = Pattern::Make(pwd.pw_name);
        if (!pwdPattern.HasValue())
        {
            return pwdPattern.Error();
        }

        auto groupPattern = Pattern::Make(group->gr_name);
        if (!groupPattern.HasValue())
        {
            return groupPattern.Error();
        }

        EnsureFilePermissionsParams params;
        params.filename = pwd.pw_dir;
        params.mask = 027;
        params.owner = {{std::move(pwdPattern.Value())}};
        params.group = {{std::move(groupPattern.Value())}};
        indicators.Push("EnsureFilePermissions");
        auto subResult = AuditEnsureFilePermissions(params, indicators, context);
        if (!subResult.HasValue())
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to check permissions for home directory '%s' for user '%s': %s", pwd.pw_dir, pwd.pw_name,
                subResult.Error().message.c_str());
            OSConfigTelemetryStatusTrace("AuditEnsureFilePermissions", subResult.Error().code);
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

Result<Status> RemediateEnsureInteractiveUsersHomeDirectoriesAreConfigured(IndicatorsTree& indicators, ContextInterface& context)
{
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
                    OSConfigTelemetryStatusTrace("mkdir", status);
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

        auto pwdPattern = Pattern::Make(pwd.pw_name);
        if (!pwdPattern.HasValue())
        {
            return pwdPattern.Error();
        }

        auto groupPattern = Pattern::Make(group->gr_name);
        if (!groupPattern.HasValue())
        {
            return groupPattern.Error();
        }

        EnsureFilePermissionsParams params;
        params.filename = pwd.pw_dir;
        params.mask = 027;
        params.owner = {{std::move(pwdPattern.Value())}};
        params.group = {{std::move(groupPattern.Value())}};
        indicators.Push("EnsureFilePermissions");
        auto subResult = RemediateEnsureFilePermissions(params, indicators, context);
        if (!subResult.HasValue())
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to remediate permissions for home directory '%s' for user '%s': %s", pwd.pw_dir,
                pwd.pw_name, subResult.Error().message.c_str());
            OSConfigTelemetryStatusTrace("RemediateEnsureFilePermissionsHelper", subResult.Error().code);
            result = Status::NonCompliant;
        }
        indicators.Back().status = subResult.Value();
        indicators.Pop();
    }
    return result;
}

} // namespace ComplianceEngine
