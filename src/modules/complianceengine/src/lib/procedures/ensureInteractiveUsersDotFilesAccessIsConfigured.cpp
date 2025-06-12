// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "IterateUsers.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include <FilePermissionsHelpers.h>
#include <FileTreeWalk.h>
#include <Result.h>
#include <fcntl.h>
#include <fstream>
#include <ftw.h>
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

namespace
{

constexpr const char* etcShellsPath = "/etc/shells";
Result<set<string>> ListValidShells(ContextInterface& context)
{
    set<string> validShells;
    ifstream shellsFile("/etc/shells");
    if (!shellsFile.is_open())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to open %s file", etcShellsPath);
        return Error(std::string("Failed to open ") + etcShellsPath + " file", EINVAL);
    }
    string line;
    while (std::getline(shellsFile, line))
    {
        if (line.empty() || line[0] == '#')
        {
            OsConfigLogDebug(context.GetLogHandle(), "Ignoring %s entry: %s", etcShellsPath, line.c_str());
            continue;
        }
        const auto pos = line.find("nologin");
        if (pos != std::string::npos)
        {
            OsConfigLogDebug(context.GetLogHandle(), "Ignoring %s entry: %s", etcShellsPath, line.c_str());
            continue;
        }

        validShells.insert(line);
    }
    return validShells;
}
} // anonymous namespace

AUDIT_FN(EnsureInteractiveUsersDotFilesAccessIsConfigured)
{
    UNUSED(args);

    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto status = Status::Compliant;
    UsersRange users;
    for (const auto& pwd : users)
    {
        const auto shell = string(pwd->pw_shell);
        const auto it = validShells->find(shell);
        if (it == validShells->end())
        {
            OsConfigLogDebug(context.GetLogHandle(), "User '%s' has shell '%s' not listed in %s", pwd->pw_name, pwd->pw_shell, etcShellsPath);
            continue;
        }

        const auto* group = getgrgid(pwd->pw_gid);
        if (nullptr == group)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to get group for user '%s': %s", pwd->pw_name, strerror(errno));
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
                return indicators.NonCompliant("'" + filename + "' exists in home directory '" + pwd->pw_dir + "'");
            }

            Result<Status> result = Status::Compliant;
            const auto path = directory + "/" + filename;

            // Performs a file permissions check and updates the result in case of error or non-compliance
            auto checkFile = [pwd, group, &path, &indicators, &context, &result](std::string mask) {
                map<string, string> arguments = {{"owner", pwd->pw_name}, {"group", group->gr_name}, {"mask", std::move(mask)}};
                indicators.Push("AuditEnsureFilePermissionsHelper");
                auto subResult = AuditEnsureFilePermissionsHelper(path, arguments, indicators, context);
                indicators.Pop();
                if (!subResult.HasValue())
                {
                    OsConfigLogError(context.GetLogHandle(), "Failed to check permissions for file '%s': %s", path.c_str(), subResult.Error().message.c_str());
                    result = subResult.Error();
                }

                if (subResult.Value() == Status::NonCompliant)
                {
                    result = subResult.Value();
                }
            };

            if (result.HasValue() && filename == ".netrc")
            {
                checkFile("177");
            }

            if (result.HasValue() && filename == ".bash_history")
            {
                checkFile("177");
            }

            if (result.HasValue())
            {
                checkFile("133");
            }

            return result;
        };

        if (FileTreeWalk(pwd->pw_dir, ftwCallback, BreakOnNonCompliant::False, context).Value() == Status::NonCompliant)
        {
            OsConfigLogDebug(context.GetLogHandle(), "Directory validation for user %s id %d returned NonCompliant, but continuing", pwd->pw_name, pwd->pw_uid);
            status = Status::NonCompliant;
        }
    }

    return status;
}

REMEDIATE_FN(EnsureInteractiveUsersDotFilesAccessIsConfigured)
{
    UNUSED(args);

    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto status = Status::Compliant;
    UsersRange users;
    for (const auto& user : users)
    {
        const auto shell = string(user->pw_shell);
        const auto it = validShells->find(shell);
        if (it == validShells->end())
        {
            OsConfigLogDebug(context.GetLogHandle(), "User '%s' has shell '%s' not listed in %s", user->pw_name, user->pw_shell, etcShellsPath);
            return Status::Compliant;
        }

        const auto* group = getgrgid(user->pw_gid);
        if (nullptr == group)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to get group for user '%s': %s", user->pw_name, strerror(errno));
            return Error(string("Failed to get group for user: ") + strerror(errno), errno);
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
                return indicators.NonCompliant("'" + filename + "' exists in home directory '" + user->pw_dir + "'");
            }

            Result<Status> result = Status::Compliant;
            const auto path = directory + "/" + filename;

            // Performs a file permissions check and updates the result in case of error or non-compliance
            auto remediateFile = [user, group, &path, &indicators, &context, &result](std::string mask) {
                map<string, string> arguments = {{"owner", user->pw_name}, {"group", group->gr_name}, {"mask", std::move(mask)}};
                indicators.Push("RemediateEnsureFilePermissionsHelper");
                auto subResult = RemediateEnsureFilePermissionsHelper(path, arguments, indicators, context);
                indicators.Pop();
                if (!subResult.HasValue())
                {
                    OsConfigLogError(context.GetLogHandle(), "Failed to remediate permissions for file '%s': %s", path.c_str(),
                        subResult.Error().message.c_str());
                    result = subResult.Error();
                }

                if (subResult.Value() == Status::NonCompliant)
                {
                    result = subResult.Value();
                }
            };

            if (result.HasValue() && (filename == ".netrc" || filename == ".bash_history"))
            {
                remediateFile("177");
            }

            if (result.HasValue())
            {
                // Ths file starts with a dot, so we need to check the permissions
                remediateFile("133");
            }

            return result;
        };

        OsConfigLogError(context.GetLogHandle(), "FileTreeWalk  user %s id %d directory %s,", user->pw_name, user->pw_uid, user->pw_dir);
        if (FileTreeWalk(user->pw_dir, ftwCallback, BreakOnNonCompliant::False, context).Value() == Status::NonCompliant)
        {
            OsConfigLogError(context.GetLogHandle(), "Directory validation for user %s id %d returned NonCompliant, but continuing", user->pw_name, user->pw_uid);
            status = Status::NonCompliant;
        }
    }

    return status;
}

} // namespace ComplianceEngine
