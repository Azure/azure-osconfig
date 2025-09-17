// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <FileTreeWalk.h>
#include <Internal.h>
#include <dirent.h>

namespace ComplianceEngine
{
namespace
{
constexpr size_t maxDepth = 32;
Result<Status> FileTreeWalk(const std::string& path, FtwCallback callback, BreakOnNonCompliant breakOnNonCompliant, ContextInterface& context, size_t depth)
{
    if (depth > maxDepth)
    {
        return Error("Maximum recursion depth reached");
    }

    auto* dir = opendir(path.c_str());
    if (nullptr == dir)
    {
        int status = errno;
        if (ENOENT == status)
        {
            return Status::Compliant;
        }

        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "opendir", status);
        OsConfigLogError(context.GetLogHandle(), "Failed to open directory '%s': %s", path.c_str(), strerror(status));
        return Error("Failed to open directory '" + path + "': " + strerror(status), status);
    }

    Result<Status> result = Status::Compliant;
    Result<Status> subResult = Status::Compliant;
    struct dirent* entry = nullptr;
    for (errno = 0, entry = readdir(dir); nullptr != entry; errno = 0, entry = readdir(dir))
    {
        if (entry->d_type == DT_DIR)
        {
            if ((0 == strcmp(entry->d_name, ".")) || (0 == strcmp(entry->d_name, "..")))
            {
                continue;
            }

            // Recursively call FTW for subdirectories
            subResult = FileTreeWalk(path + "/" + entry->d_name, callback, breakOnNonCompliant, context, depth + 1);
            if (!subResult.HasValue())
            {
                OsConfigLogDebug(context.GetLogHandle(), "Callback returned an error: %s", subResult.Error().message.c_str());
                return subResult.Error();
            }

            if (subResult.Value() != Status::Compliant)
            {
                result = Status::NonCompliant;
                if (breakOnNonCompliant == BreakOnNonCompliant::True)
                {
                    OsConfigLogDebug(context.GetLogHandle(), "Callback returned NonCompliant status, stopping iteration");
                    break;
                }
            }
        }

        struct stat sb;
        std::string directory = path + "/" + entry->d_name;
        OsConfigLogDebug(context.GetLogHandle(), "Checking file: '%s'", directory.c_str());
        if (0 != lstat(directory.c_str(), &sb))
        {
            int status = errno;
            OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "lstat", status);
            OsConfigLogError(context.GetLogHandle(), "Failed to lstat '%s': %s", directory.c_str(), strerror(status));
            result = Error("Failed to lstat '" + directory + "': " + strerror(status), status);
            break;
        }

        subResult = callback(path, entry->d_name, sb);
        if (!subResult.HasValue())
        {
            OsConfigLogDebug(context.GetLogHandle(), "Callback returned an error: %s", subResult.Error().message.c_str());
            return subResult.Error();
        }

        if (subResult.Value() != Status::Compliant)
        {
            result = Status::NonCompliant;
            if (breakOnNonCompliant == BreakOnNonCompliant::True)
            {
                OsConfigLogDebug(context.GetLogHandle(), "Callback returned NonCompliant status, stopping iteration");
                break;
            }
        }
    }

    int status = errno;
    closedir(dir);
    if (!result.HasValue())
    {
        OsConfigLogDebug(context.GetLogHandle(), "Iteration failed with an error: %s", result.Error().message.c_str());
        return result.Error();
    }

    if (0 != status)
    {
        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "readdir", status);
        OsConfigLogError(context.GetLogHandle(), "Failed to iterate directory '%s': %s", path.c_str(), strerror(status));
        return Error("Failed to iterate directory '" + path + "': " + strerror(status), status);
    }

    return result;
}
} // anonymous namespace

Result<Status> FileTreeWalk(const std::string& path, FtwCallback callable, BreakOnNonCompliant breakOnNonCompliant, ContextInterface& context)
{
    return FileTreeWalk(path, callable, breakOnNonCompliant, context, 0);
}
} // namespace ComplianceEngine
