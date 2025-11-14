// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <EnsureFilePermissions.h>
#include <EnsureInteractiveUsersDotFilesAccessIsConfigured.h>
#include <Evaluator.h>
#include <FileTreeWalk.h>
#include <ListValidShells.h>
#include <Result.h>
#include <Telemetry.h>
#include <UsersIterator.h>
#include <fcntl.h>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace ComplianceEngine
{
using std::map;
using std::string;

Result<Status> AuditEnsureInteractiveUsersDotFilesAccessIsConfigured(IndicatorsTree& indicators, ContextInterface& context)
{
    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto status = Status::Compliant;
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
            OsConfigLogDebug(context.GetLogHandle(), "User '%s' has shell '%s' not listed in valid shells", pwd.pw_name, pwd.pw_shell);
            continue;
        }

        const auto* group = getgrgid(pwd.pw_gid);
        if (nullptr == group)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to get group for user '%s': %s", pwd.pw_name, strerror(errno));
            OSConfigTelemetryStatusTrace("getgrgid", errno);
            return Error(string("Failed to get group for user: ") + strerror(errno), errno);
        }

        auto ftwCallback = [pwd, group, &indicators, &context](const string& directory, const std::string& filename, const struct stat& st) -> Result<Status> {
            if (!S_ISREG(st.st_mode))
            {
                OsConfigLogDebug(context.GetLogHandle(), "Skipping non-regular file '%s'", filename.c_str());
                return Status::Compliant;
            }

            if (filename.find(".") != 0)
            {
                OsConfigLogDebug(context.GetLogHandle(), "Skipping entry '%s' as its name doesn't start with '.'", filename.c_str());
                return Status::Compliant;
            }

            if (filename == ".forward" || filename == ".rhost")
            {
                return indicators.NonCompliant("'" + filename + "' exists in home directory '" + pwd.pw_dir + "'");
            }

            Result<Status> result = Status::Compliant;
            const auto path = directory + "/" + filename;

            // Performs a file permissions check and updates the result in case of error or non-compliance
            auto checkFile = [pwd, group, &path, &indicators, &context, &result](const mode_t mask) {
                auto groupPattern = Pattern::Make(group->gr_name);
                if (!groupPattern.HasValue())
                {
                    result = groupPattern.Error();
                    return;
                }

                auto pwdPattern = Pattern::Make(pwd.pw_name);
                if (!pwdPattern.HasValue())
                {
                    result = pwdPattern.Error();
                    return;
                }

                EnsureFilePermissionsParams params;
                params.filename = path;
                params.owner = {{std::move(pwdPattern.Value())}};
                params.group = {{std::move(groupPattern.Value())}};
                params.mask = mask;
                indicators.Push("AuditEnsureFilePermissions");
                auto subResult = AuditEnsureFilePermissions(params, indicators, context);
                if (!subResult.HasValue())
                {
                    OsConfigLogError(context.GetLogHandle(), "Failed to check permissions for file '%s': %s", path.c_str(), subResult.Error().message.c_str());
                    OSConfigTelemetryStatusTrace("AuditEnsureFilePermissions", subResult.Error().code);
                    result = subResult.Error();
                    return;
                }
                indicators.Back().status = subResult.Value();
                indicators.Pop();

                if (subResult.Value() == Status::NonCompliant)
                {
                    result = subResult.Value();
                }
            };

            if (result.HasValue() && filename == ".netrc")
            {
                checkFile(0177);
            }

            if (result.HasValue() && filename == ".bash_history")
            {
                checkFile(0177);
            }

            if (result.HasValue())
            {
                checkFile(0133);
            }

            return result;
        };

        auto result = FileTreeWalk(pwd.pw_dir, ftwCallback, BreakOnNonCompliant::True, context);
        if (!result.HasValue() || result.Value() == Status::NonCompliant)
        {
            OsConfigLogDebug(context.GetLogHandle(), "Directory validation for user %s id %d returned NonCompliant, but continuing", pwd.pw_name, pwd.pw_uid);
            status = Status::NonCompliant;
        }
    }

    return status;
}

Result<Status> RemediateEnsureInteractiveUsersDotFilesAccessIsConfigured(IndicatorsTree& indicators, ContextInterface& context)
{
    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto status = Status::Compliant;
    auto users = UsersRange::Make(context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& user : users.Value())
    {
        const auto shell = string(user.pw_shell);
        const auto it = validShells->find(shell);
        if (it == validShells->end())
        {
            OsConfigLogDebug(context.GetLogHandle(), "User '%s' has shell '%s' not listed in valid shells", user.pw_name, user.pw_shell);
            continue;
        }

        const auto* group = getgrgid(user.pw_gid);
        if (nullptr == group)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to get group for user '%s': %s", user.pw_name, strerror(errno));
            OSConfigTelemetryStatusTrace("getgrgid", errno);
            status = Status::NonCompliant;
        }

        auto ftwCallback = [user, group, &indicators, &context](const string& directory, const std::string& filename, const struct stat& st) -> Result<Status> {
            if (!S_ISREG(st.st_mode))
            {
                OsConfigLogDebug(context.GetLogHandle(), "Skipping non-regular file '%s'", filename.c_str());
                return Status::Compliant;
            }

            if (filename.find(".") != 0)
            {
                OsConfigLogDebug(context.GetLogHandle(), "Skipping entry '%s' as its name doesn't start with '.'", filename.c_str());
                return Status::Compliant;
            }

            if (filename == ".forward" || filename == ".rhost")
            {
                // We don't want to remove user files, the remediation will always fail here.
                return indicators.NonCompliant("'" + filename + "' exists in home directory '" + user.pw_dir + "'");
            }

            Result<Status> result = Status::Compliant;
            const auto path = directory + "/" + filename;

            // Performs a file permissions check and updates the result in case of error or non-compliance
            auto remediateFile = [user, group, &path, &indicators, &context, &result](const mode_t mask) {
                auto pwdPattern = Pattern::Make(user.pw_name);
                if (!pwdPattern.HasValue())
                {
                    result = pwdPattern.Error();
                    return;
                }
                auto groupPattern = Pattern::Make(group->gr_name);
                if (!groupPattern.HasValue())
                {
                    result = groupPattern.Error();
                    return;
                }
                EnsureFilePermissionsParams params;
                params.filename = path;
                params.owner = {{pwdPattern.Value()}};
                params.group = {{groupPattern.Value()}};
                params.mask = mask;
                indicators.Push("RemediateEnsureFilePermissions");
                auto subResult = RemediateEnsureFilePermissions(params, indicators, context);
                if (!subResult.HasValue())
                {
                    OsConfigLogError(context.GetLogHandle(), "Failed to remediate permissions for file '%s': %s", path.c_str(),
                        subResult.Error().message.c_str());
                    OSConfigTelemetryStatusTrace("RemediateEnsureFilePermissionsHelper", subResult.Error().code);
                    result = subResult.Error();
                    return;
                }
                indicators.Back().status = subResult.Value();
                indicators.Pop();

                if (subResult.Value() == Status::NonCompliant)
                {
                    result = subResult.Value();
                }
            };

            if (result.HasValue() && (filename == ".netrc" || filename == ".bash_history"))
            {
                remediateFile(0177);
            }

            if (result.HasValue())
            {
                // Ths file starts with a dot, so we need to check the permissions
                remediateFile(0133);
            }

            return result;
        };

        auto result = FileTreeWalk(user.pw_dir, ftwCallback, BreakOnNonCompliant::False, context);
        if (!result.HasValue() || result.Value() == Status::NonCompliant)
        {
            OsConfigLogError(context.GetLogHandle(), "Directory validation for user %s id %d returned NonCompliant, but continuing", user.pw_name, user.pw_uid);
            status = Status::NonCompliant;
        }
    }

    return status;
}

} // namespace ComplianceEngine
