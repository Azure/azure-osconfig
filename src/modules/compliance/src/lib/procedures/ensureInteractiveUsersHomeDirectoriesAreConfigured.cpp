// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "IterateUsers.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <fcntl.h>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace compliance
{
using std::ifstream;
using std::map;
using std::ostringstream;
using std::set;
using std::string;

Result<bool> AuditEnsureFilePermissions(map<string, string>, std::ostringstream&, OsConfigLogHandle);
Result<bool> RemediateEnsureFilePermissions(map<string, string>, std::ostringstream&, OsConfigLogHandle);

namespace
{
Result<set<string>> ListValidShells(OsConfigLogHandle log)
{
    set<string> validShells;
    ifstream shellsFile("/etc/shells");
    if (!shellsFile.is_open())
    {
        OsConfigLogError(log, "Failed to open /etc/shells file");
        return Error("Failed to open /etc/shells file", EINVAL);
    }
    string line;
    while (std::getline(shellsFile, line))
    {
        if (line.empty() || line[0] == '#')
        {
            OsConfigLogDebug(log, "Ignoring /etc/shells entry: %s", line.c_str());
            continue;
        }
        const auto pos = line.find("nologin");
        if (pos != std::string::npos)
        {
            OsConfigLogDebug(log, "Ignoring /etc/shells entry: %s", line.c_str());
            continue;
        }

        validShells.insert(line);
    }
    return validShells;
}
} // anonymous namespace

AUDIT_FN(EnsureInteractiveUsersHomeDirectoriesAreConfigured)
{
    UNUSED(args);

    const auto validShells = ListValidShells(log);
    if (!validShells.HasValue())
    {
        OsConfigLogError(log, "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto cb = [&validShells, &logstream, log](const passwd* pwd) -> Result<bool> {
        const auto shell = string(pwd->pw_shell);
        const auto it = validShells->find(shell);
        if (it == validShells->end())
        {
            OsConfigLogDebug(log, "User '%s' has shell '%s' not listed in /etc/shells", pwd->pw_name, pwd->pw_shell);
            return true;
        }

        struct stat st;
        if (stat(pwd->pw_dir, &st) != 0)
        {
            int status = errno;
            if (status == ENOENT)
            {
                OsConfigLogDebug(log, "User '%s' has home directory '%s' which does not exist", pwd->pw_name, pwd->pw_dir);
                logstream << "User's '" << pwd->pw_name << "' home directory '" << pwd->pw_dir << "' does not exist";
                return false;
            }
            else
            {
                OsConfigLogError(log, "Failed to stat home directory '%s' for user '%s': %s", pwd->pw_dir, pwd->pw_name, strerror(status));
                return Error(string("Failed to stat home directory: ") + strerror(status), status);
            }
        }

        map<string, string> args = {{"filename", pwd->pw_dir}, {"mask", "027"}, {"owner", pwd->pw_name}, {"group", pwd->pw_name}};
        std::ostringstream subLogstream;
        auto subResult = AuditEnsureFilePermissions(std::move(args), subLogstream, log);
        if (!subResult.HasValue())
        {
            OsConfigLogError(log, "Failed to check permissions for home directory '%s' for user '%s': %s", pwd->pw_dir, pwd->pw_name,
                subResult.Error().message.c_str());
            return subResult.Error();
        }

        if (!subResult.Value())
        {
            OsConfigLogInfo(log, "User '%s' has home directory '%s' with incorrect permissions", pwd->pw_name, pwd->pw_dir);
            logstream << "User's '" << pwd->pw_name << "' home directory '" << pwd->pw_dir << "' has incorrect permissions ";
            return false;
        }
        return true;
    };

    return IterateUsers(cb, BreakOnFalse::False, log);
}

REMEDIATE_FN(EnsureInteractiveUsersHomeDirectoriesAreConfigured)
{
    UNUSED(args);
    UNUSED(logstream);

    const auto validShells = ListValidShells(log);
    if (!validShells.HasValue())
    {
        OsConfigLogError(log, "Failed to get valid shells: %s", validShells.Error().message.c_str());
        return validShells.Error();
    }

    auto cb = [&validShells, log](const passwd* pwd) -> Result<bool> {
        const auto shell = string(pwd->pw_shell);
        const auto it = validShells->find(shell);
        if (it == validShells->end())
        {
            OsConfigLogDebug(log, "User '%s' has shell '%s' not in /etc/shells", pwd->pw_name, pwd->pw_shell);
            return true;
        }

        struct stat st;
        if (stat(pwd->pw_dir, &st) != 0)
        {
            int status = errno;
            OsConfigLogDebug(log, "stat failed for home directory '%s' for user '%s': %s", pwd->pw_dir, pwd->pw_name, strerror(status));
            if (status == ENOENT)
            {
                // Home directory does not exist, so we need to create it
                if (0 != mkdir(pwd->pw_dir, 0750))
                {
                    status = errno;
                    OsConfigLogError(log, "Failed to create home directory '%s' for user '%s': %s", pwd->pw_dir, pwd->pw_name, strerror(status));
                    return Error(string("Failed to create home directory: ") + strerror(status), status);
                }
            }
            else
            {
                OsConfigLogError(log, "Failed to stat home directory '%s' for user '%s': %s", pwd->pw_dir, pwd->pw_name, strerror(status));
                return Error(string("Failed to stat home directory: ") + strerror(status), status);
            }
        }

        map<string, string> args = {{"filename", pwd->pw_dir}, {"mask", "027"}, {"owner", pwd->pw_name}, {"group", pwd->pw_name}};
        std::ostringstream subLogstream;
        auto subResult = RemediateEnsureFilePermissions(std::move(args), subLogstream, log);
        if (!subResult.HasValue())
        {
            OsConfigLogError(log, "Failed to remediate permissions for home directory '%s' for user '%s': %s", pwd->pw_dir, pwd->pw_name,
                subResult.Error().message.c_str());
            return subResult.Error();
        }

        return subResult.Value();
    };

    return IterateUsers(cb, BreakOnFalse::False, log);
}

} // namespace compliance
