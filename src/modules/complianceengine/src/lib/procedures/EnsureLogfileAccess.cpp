// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Bindings.h>
#include <EnsureFilePermissions.h>
#include <EnsureLogfileAccess.h>
#include <Evaluator.h>
#include <FileTreeWalk.h>
#include <Result.h>
#include <Telemetry.h>
#include <fnmatch.h>
#include <map>
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

// Default mask for files that don't match any pattern
// TODO(wpk) add the magic related to daemon-owned files.
const std::map<std::string, std::string> g_defaultLogfileArgs = {{"owner", "root|syslog"}, {"group", "root|adm"}, {"mask", "0137"}};

EnsureFilePermissionsParams GetFilePermissionsParams(const std::string& filename, std::map<std::string, std::string> args)
{
    args["filename"] = filename;
    auto result = BindingsImpl::ParseArguments<EnsureFilePermissionsParams>(args);
    if (!result.HasValue())
    {
        throw std::logic_error("invalid static permissions map");
    }
    return result.Value();
}

EnsureFilePermissionsParams GetFilePermissionArgs(const std::string& filename, const std::string& fullPath)
{
    for (const auto& pattern : g_logfilePatterns)
    {
        if (fnmatch(pattern.first.c_str(), filename.c_str(), FNM_CASEFOLD) == 0)
        {
            return GetFilePermissionsParams(fullPath, pattern.second);
        }
    }

    return GetFilePermissionsParams(fullPath, g_defaultLogfileArgs);
}

Result<Status> ProcessLogfile(const std::string& path, const std::string& filename, const struct stat& statInfo, IndicatorsTree& indicators,
    ContextInterface& context, bool remediate)
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
    const auto params = GetFilePermissionArgs(filename, fullPath);

    OsConfigLogDebug(context.GetLogHandle(), "Processing logfile: %s with pattern-matched permissions", fullPath.c_str());

    indicators.Push("FilePermissionsCheck for " + fullPath);

    Result<Status> result = remediate ? RemediateEnsureFilePermissions(params, indicators, context) : AuditEnsureFilePermissions(params, indicators, context);
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

Result<Status> AuditEnsureLogfileAccess(const EnsureLogfileAccessParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.path.HasValue());
    OsConfigLogInfo(context.GetLogHandle(), "Auditing logfile access permissions in directory: %s", params.path->c_str());

    auto callback = [&indicators, &context](const std::string& dirPath, const std::string& filename, const struct stat& statInfo) -> Result<Status> {
        return ProcessLogfile(dirPath, filename, statInfo, indicators, context, false);
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

Result<Status> RemediateEnsureLogfileAccess(const EnsureLogfileAccessParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.path.HasValue());
    OsConfigLogInfo(context.GetLogHandle(), "Remediating logfile access permissions in directory: %s", params.path->c_str());

    auto callback = [&indicators, &context](const std::string& dirPath, const std::string& filename, const struct stat& statInfo) -> Result<Status> {
        return ProcessLogfile(dirPath, filename, statInfo, indicators, context, true);
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
