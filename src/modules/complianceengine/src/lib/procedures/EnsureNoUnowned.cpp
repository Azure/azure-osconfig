// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// EnsureNoUnowned: Fails if any file in the scanned filesystem snapshot has a UID
// that is not present in /etc/passwd (as enumerated by UsersIterator). Stops at
// the first unowned file (early exit) and returns NonCompliant. If all files
// are owned by known UIDs, returns Compliant.

#include <CommonUtils.h>
#include <EnsureNoUnowned.h>
#include <Evaluator.h>
#include <FilesystemScanner.h>
#include <GroupsIterator.h>
#include <UsersIterator.h>
#include <fnmatch.h>
#include <set>
#include <sys/stat.h>

namespace ComplianceEngine
{
static constexpr int maxUnowned = 3;

Result<Status> AuditEnsureNoUnowned(IndicatorsTree& indicators, ContextInterface& context)
{
    const std::vector<std::string> ommited_paths = {"/run/*", "/proc/*", "*/containerd/*", "*/kubelet/*", "/sys/fs/cgroup/memory/*", "/var/*/private/*"};
    // Build set of known uids and gids
    std::set<uid_t> knownUids;
    auto usersRange = UsersRange::Make(context.GetSpecialFilePath("/etc/passwd"), context.GetLogHandle());
    if (!usersRange.HasValue())
    {
        return usersRange.Error();
    }
    for (const auto& pw : usersRange.Value())
    {
        knownUids.insert(pw.pw_uid);
    }
    std::set<uid_t> knownGids;
    auto groupsRange = GroupsRange::Make(context.GetSpecialFilePath("/etc/group"), context.GetLogHandle());
    if (!groupsRange.HasValue())
    {
        return groupsRange.Error();
    }
    for (const auto& gr : groupsRange.Value())
    {
        knownGids.insert(gr.gr_gid);
    }

    // Get filesystem snapshot
    FilesystemScanner& scanner = context.GetFilesystemScanner();
    auto fsRes = scanner.GetFullFilesystem();
    if (!fsRes)
    {
        return fsRes.Error();
    }
    const auto& entries = fsRes.Value()->entries;

    int unowned = 0;
    for (const auto& kv : entries)
    {
        if (unowned >= maxUnowned)
        {
            break;
        }
        const auto& path = kv.first;
        const auto& st = kv.second.st;
        bool omit = false;
        for (const auto& pattern : ommited_paths)
        {
            if (fnmatch(pattern.c_str(), path.c_str(), 0) == 0)
            {
                OsConfigLogDebug(context.GetLogHandle(), "Skipping path %s matching omit pattern %s", path.c_str(), pattern.c_str());
                omit = true;
                break;
            }
        }
        if (omit)
        {
            continue;
        }
        if (knownUids.find(st.st_uid) == knownUids.end())
        {
            indicators.NonCompliant("Unowned file '" + path + "' with uid " + std::to_string(static_cast<long long>(st.st_uid)));
            unowned++;
        }
        if (knownGids.find(st.st_gid) == knownGids.end())
        {
            indicators.NonCompliant("Unowned file '" + path + "' with gid " + std::to_string(static_cast<long long>(st.st_gid)));
            unowned++;
        }
    }
    if (unowned > 0)
    {
        return indicators.NonCompliant("Unowned files found in the filesystem (up to " + std::to_string(maxUnowned) + " listed)");
    }
    return indicators.Compliant("All files owned by known users");
}

} // namespace ComplianceEngine
