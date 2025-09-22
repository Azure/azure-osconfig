// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Optional.h"
#include "Result.h"

#include <map>
#include <memory>
#include <string>
#include <sys/stat.h>

namespace ComplianceEngine
{

class FilesystemScanner
{
public:
    struct FSEntry
    {
        struct stat st;
    };
    struct FSCache
    {
        time_t scan_start_time;
        time_t scan_end_time;
        std::map<std::string, FSEntry> entries; // key = path
    };

    // Construct with root directory, cache file, lock file and timeout values (all in seconds):
    //  softTimeout: serve stale data but trigger background refresh when age >= soft.
    //  hardTimeout: treat cache as unusable when age >= hard; start refresh and return error until replaced (unless wait succeeds).
    //  waitTimeoutSeconds: optional max seconds to poll for a newly built cache when none usable (initial or hard-expired case).
    explicit FilesystemScanner(std::string rootDir, std::string cachePath, std::string lockPath, time_t softTimeoutSeconds, time_t hardTimeoutSeconds,
        time_t waitTimeoutSeconds);

    // Returns shared_ptr view of full filesystem cache (may trigger background scan per timeout rules)
    Result<std::shared_ptr<const FSCache>> GetFullFilesystem();
    // Filtered retrieval: returns subset map of entries where (if has_perms set) all bits are present in st_mode
    // AND (if no_perms set) none of those bits are present.
    Result<std::map<std::string, FSEntry>> GetFilteredFilesystemEntries(Optional<mode_t> has_perms, Optional<mode_t> no_perms);
    // No synchronous scanning API: all scans occur in background processes.

private:
    void LoadCache(); // Attempt to load cache file; ignores stale/invalid formats.

    std::string root;
    std::string cachePath;
    std::string lockPath;
    std::shared_ptr<FSCache> m_cache; // Shared so callers can retain view while refresh occurs.
    time_t m_softTimeout = 0;         // Serve stale + refresh when exceeded (0 = disabled)
    time_t m_hardTimeout = 0;         // Invalidate when exceeded (0 = disabled); >= soft if both set
    time_t m_waitTimeout = 0;         // Optional polling window for initial/hard-expired rebuild (0 = no wait)
};
} // namespace ComplianceEngine
