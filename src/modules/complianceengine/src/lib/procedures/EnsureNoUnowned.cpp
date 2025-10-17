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
#include <UsersIterator.h>
#include <fnmatch.h>
#include <set>
#include <sys/stat.h>

namespace ComplianceEngine
{

Result<Status> AuditEnsureNoUnowned(IndicatorsTree& indicators, ContextInterface& context)
{
    const std::vector<std::string> ommited_paths = {"/run/*", "/proc/*", "*/containerd/*", "*/kubelet/*", "/sys/fs/cgroup/memory/*", "/var/*/private/*"};
    // Build set of known user IDs
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

    // Get filesystem snapshot
    FilesystemScanner& scanner = context.GetFilesystemScanner();
    auto fsRes = scanner.GetFullFilesystem();
    if (!fsRes)
    {
        return fsRes.Error();
    }
    const auto& entries = fsRes.Value()->entries;

    std::vector<std::string> unowned;
    unowned.reserve(3);
    for (const auto& kv : entries)
    {
        if (unowned.size() >= 3)
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
            unowned.push_back("uid=" + std::to_string(static_cast<long long>(st.st_uid)) + " path=" + path);
        }
    }
    if (!unowned.empty())
    {
        std::string msg = "Unowned files (up to 3): ";
        for (size_t i = 0; i < unowned.size(); ++i)
        {
            if (i)
                msg += "; ";
            msg += unowned[i];
        }
        return indicators.NonCompliant(msg);
    }
    return indicators.Compliant("All files owned by known users");
}

} // namespace ComplianceEngine
