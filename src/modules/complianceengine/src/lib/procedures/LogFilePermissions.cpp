// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <Bindings.h>
#include <Evaluator.h>
#include <FilePermissions.h>
#include <FileTreeWalk.h>
#include <ListValidShells.h>
#include <LogFilePermissions.h>
#include <Result.h>
#include <Telemetry.h>
#include <UsersIterator.h>
#include <fnmatch.h>
#include <map>
#include <pwd.h>
#include <set>
#include <string>
#include <sys/stat.h>

namespace ComplianceEngine
{
namespace
{

// Global map of fnmatch patterns to file permission arguments
// Pattern -> {args}
const std::map<std::string, std::map<std::string, std::string>> g_logfilePatterns = {
    {"lastlog", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"lastlog.*", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"wtmp", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"wtmp.*", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"wtmp-*", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"btmp", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"btmp.*", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"btmp-*", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"README", {{"owner", "root"}, {"group", "root|utmp"}, {"mask", "0113"}}},
    {"cloud-init.log*", {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0133"}}},
    {"localmessages*", {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0133"}}},
    {"waagent.log*", {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0133"}}},
    {"secure{,*.*,.*,-*}", {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0137"}}},
    {"auth.log", {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0137"}}},
    {"syslog", {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0137"}}},
    {"messages", {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0137"}}},
    {"sssd", {{"owner", "root|SSSD"}, {"group", "root|SSSD"}, {"mask", "0117"}}},
    {"SSSD", {{"owner", "root|SSSD"}, {"group", "root|SSSD"}, {"mask", "0117"}}},
    {"gdm", {{"owner", "root"}, {"group", "root|gdm|gdm3"}, {"mask", "0117"}}},
    {"gdm3", {{"owner", "root"}, {"group", "root|gdm|gdm3"}, {"mask", "0117"}}},
    {"*.journal", {{"owner", "root"}, {"group", "root|systemd-journal"}, {"mask", "0137"}}},
    {"*.journal~", {{"owner", "root"}, {"group", "root|systemd-journal"}, {"mask", "0137"}}},
};

// Default arguments for files that don't match any pattern.
const std::map<std::string, std::string> g_defaultLogfileArgs = {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0137"}};

using DaemonUidSet = std::set<uid_t>;

// Builds the set of uids that belong to root or a daemon/service account (one whose login shell is
// not a valid interactive shell per /etc/shells). Computed once from /etc/passwd so we don't have to
// look up each file's owner while walking the log directory.
Result<DaemonUidSet> BuildDaemonUidSet(ContextInterface& context)
{
    auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to list valid shells: %s", validShells.Error().message.c_str());
        OSConfigTelemetryStatusTrace("ListValidShells", validShells.Error().code);
        return validShells.Error();
    }

    auto users = UsersRange::Make(context.GetSpecialFilePath("/etc/passwd"), context.GetLogHandle());
    if (!users.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to enumerate users: %s", users.Error().message.c_str());
        OSConfigTelemetryStatusTrace("UsersRange", users.Error().code);
        return users.Error();
    }

    DaemonUidSet daemonUids;
    for (const auto& user : users.Value())
    {
        const std::string name = (user.pw_name != nullptr) ? user.pw_name : std::string();
        const std::string shell = (user.pw_shell != nullptr) ? user.pw_shell : std::string();
        if (name == "root" || validShells.Value().find(shell) == validShells.Value().end())
        {
            daemonUids.insert(user.pw_uid);
        }
    }

    return daemonUids;
}

FilePermissionsParams GetFilePermissionsParams(const std::string& filename, std::map<std::string, std::string> args)
{
    args["path"] = filename;
    auto result = BindingsImpl::ParseArguments<FilePermissionsParams>(args);
    if (!result.HasValue())
    {
        throw std::logic_error("invalid static permissions map");
    }
    return result.Value();
}

FilePermissionsParams GetDefaultFilePermissionArgs(const std::string& fullPath, const struct stat& statInfo, const DaemonUidSet& daemonUids, bool remediate)
{
    if (!remediate && daemonUids.find(statInfo.st_uid) != daemonUids.end())
    {
        return GetFilePermissionsParams(fullPath, {{"mask", "0137"}});
    }

    return GetFilePermissionsParams(fullPath, g_defaultLogfileArgs);
}

FilePermissionsParams GetFilePermissionArgs(const std::string& filename, const std::string& fullPath, const struct stat& statInfo,
    const DaemonUidSet& daemonUids, bool remediate)
{
    for (const auto& pattern : g_logfilePatterns)
    {
        if (fnmatch(pattern.first.c_str(), filename.c_str(), FNM_CASEFOLD) == 0)
        {
            return GetFilePermissionsParams(fullPath, pattern.second);
        }
    }

    return GetDefaultFilePermissionArgs(fullPath, statInfo, daemonUids, remediate);
}

Result<Status> ProcessLogfile(const std::string& path, const std::string& filename, const struct stat& statInfo, const DaemonUidSet& daemonUids,
    IndicatorsTree& indicators, ContextInterface& context, bool remediate)
{
    if (S_ISDIR(statInfo.st_mode))
    {
        return Status::Compliant;
    }

    if (S_ISLNK(statInfo.st_mode))
    {
        OsConfigLogDebug(context.GetLogHandle(), "Skipping symbolic link: %s/%s", path.c_str(), filename.c_str());
        return Status::Compliant;
    }

    if (!S_ISREG(statInfo.st_mode))
    {
        return Status::Compliant;
    }

    const std::string fullPath = path + "/" + filename;
    const auto params = GetFilePermissionArgs(filename, fullPath, statInfo, daemonUids, remediate);

    OsConfigLogDebug(context.GetLogHandle(), "Processing logfile: %s with pattern-matched permissions", fullPath.c_str());

    indicators.Push("FilePermissionsCheck for " + fullPath);

    Result<Status> result = remediate ? RemediateFilePermissions(params, indicators, context) : AuditFilePermissions(params, indicators, context);
    if (!result.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to %s permissions for logfile '%s': %s", remediate ? "remediate" : "audit", fullPath.c_str(),
            result.Error().message.c_str());
        OSConfigTelemetryStatusTrace(remediate ? "RemediateEnsureFilePermissionsHelper" : "AuditEnsureFilePermissionsHelper", result.Error().code);
        return result.Error();
    }
    indicators.Back().status = result.Value();
    indicators.Pop();

    if (result.Value() != Status::Compliant)
    {
        OsConfigLogInfo(context.GetLogHandle(), "Logfile %s is non-compliant", fullPath.c_str());
        return Status::NonCompliant;
    }

    return result.Value();
}

} // anonymous namespace

Result<Status> AuditLogFilePermissions(const LogFilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.path.HasValue());
    OsConfigLogInfo(context.GetLogHandle(), "Auditing logfile access permissions in directory: %s", params.path->c_str());

    auto daemonUids = BuildDaemonUidSet(context);
    if (!daemonUids.HasValue())
    {
        return daemonUids.Error();
    }

    auto callback = [&daemonUids, &indicators, &context](const std::string& dirPath, const std::string& filename, const struct stat& statInfo) -> Result<Status> {
        return ProcessLogfile(dirPath, filename, statInfo, daemonUids.Value(), indicators, context, false);
    };

    auto result = FileTreeWalk(params.path.Value(), callback, BreakOnNonCompliant::False, context);

    if (!result.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to walk log directory '%s': %s", params.path->c_str(), result.Error().message.c_str());
        OSConfigTelemetryStatusTrace("FileTreeWalk", result.Error().code);
        return result.Error();
    }

    if (result.Value() == Status::Compliant)
    {
        return indicators.Compliant("All logfiles in " + params.path.Value() + " have correct access permissions");
    }
    else
    {
        return indicators.NonCompliant("One or more logfiles in " + params.path.Value() + " have incorrect access permissions");
    }
}

Result<Status> RemediateLogFilePermissions(const LogFilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.path.HasValue());
    OsConfigLogInfo(context.GetLogHandle(), "Remediating logfile access permissions in directory: %s", params.path->c_str());

    auto daemonUids = BuildDaemonUidSet(context);
    if (!daemonUids.HasValue())
    {
        return daemonUids.Error();
    }

    auto callback = [&daemonUids, &indicators, &context](const std::string& dirPath, const std::string& filename, const struct stat& statInfo) -> Result<Status> {
        return ProcessLogfile(dirPath, filename, statInfo, daemonUids.Value(), indicators, context, true);
    };

    auto result = FileTreeWalk(params.path.Value(), callback, BreakOnNonCompliant::False, context);

    if (!result.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to walk log directory '%s': %s", params.path->c_str(), result.Error().message.c_str());
        OSConfigTelemetryStatusTrace("FileTreeWalk", result.Error().code);
        return result.Error();
    }

    if (result.Value() == Status::Compliant)
    {
        return indicators.Compliant("Successfully set correct access permissions for all logfiles in " + params.path.Value());
    }
    else
    {
        return indicators.NonCompliant("Failed to set correct access permissions for one or more logfiles in " + params.path.Value());
    }
}

} // namespace ComplianceEngine
