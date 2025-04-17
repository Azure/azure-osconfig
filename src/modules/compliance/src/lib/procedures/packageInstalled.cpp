// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <cstdio>
#include <fstream>
#include <functional>
#include <linux/limits.h>
#include <set>
#include <string>
#include <unistd.h>

namespace
{

using compliance::ContextInterface;
using compliance::Error;
using compliance::Result;

typedef struct
{
    time_t lastUpdateTime;
    std::string packageManager;
    std::set<std::string> packageNames;
} PackageCache;

static constexpr long PACKAGELIST_TTL = 3000L;        // just shy of an hours
static constexpr long PACKAGELIST_STALE_TTL = 12600L; // 3.5 hours
static constexpr char PACKAGE_CACHE_PATH[] = "/var/lib/GuestConfig/compliancePackageCache";

class ScopeGuard
{
public:
    template <class Callable>
    ScopeGuard(Callable&& f)
        : f(std::forward<Callable>(f)){};
    void Deactivate()
    {
        active = false;
    }
    ~ScopeGuard()
    {
        if (active)
        {
            f();
        }
    }

private:
    std::function<void()> f;
    bool active{true};
};

std::string DetectPackageManager(ContextInterface& context)
{
    auto dpkg = context.ExecuteCommand("dpkg -l dpkg");
    if (dpkg.HasValue())
    {
        return "dpkg";
    }
    auto rpm = context.ExecuteCommand("rpm -qa rpm");
    if (rpm.HasValue())
    {
        return "rpm";
    }
    return "";
}

Result<PackageCache> LoadPackageCache(const std::string& path)
{
    PackageCache cache;
    const std::string pkgCacheHeader = "# PackageCache "; // "# PackageCache <packageManager>@<timestamp>\n"
    cache.lastUpdateTime = 0;
    cache.packageManager = "";
    cache.packageNames.clear();
    std::ifstream cacheFile(path);
    if (!cacheFile.is_open())
    {
        return Error("Failed to open cache file: " + path);
    }

    std::string header;
    if (!std::getline(cacheFile, header) || (0 != header.find(pkgCacheHeader, 0)))
    {
        return Error("Invalid cache file format");
    }

    auto separatorPos = header.find('@');
    if (std::string::npos == separatorPos)
    {
        return Error("Invalid cache file header format");
    }

    cache.packageManager = header.substr(pkgCacheHeader.length(), separatorPos - pkgCacheHeader.length());
    try
    {
        cache.lastUpdateTime = std::stol(header.substr(separatorPos + 1));
    }
    catch (const std::exception&)
    {
        return Error("Invalid timestamp in cache file header");
    }

    std::string packageName;
    while (std::getline(cacheFile, packageName))
    {
        if (!packageName.empty())
        {
            cache.packageNames.insert(packageName);
        }
    }

    if (cacheFile.bad())
    {
        return Error("Error reading cache file");
    }

    return cache;
}

Result<int> SavePackageCache(const PackageCache& cache, const std::string& path)
{
    std::string tempPath = path + ".tmp.XXXXXX";
    char pathTemplate[PATH_MAX];
    snprintf(pathTemplate, sizeof(pathTemplate), "%s", tempPath.c_str());
    int fd = mkstemp(pathTemplate);
    if (fd == -1)
    {
        return Error("Failed to create temporary file");
    }
    tempPath = pathTemplate;
    ScopeGuard tempFileRemover([tempPath] { std::remove(tempPath.c_str()); });
    std::ofstream tempFile(tempPath, std::ios::out | std::ios::trunc);
    close(fd);

    if (!tempFile.is_open())
    {
        return Error("Failed to open temporary file for writing: " + tempPath);
    }

    const std::string pkgCacheHeader = "# PackageCache " + cache.packageManager + "@" + std::to_string(cache.lastUpdateTime) + "\n";
    tempFile << pkgCacheHeader;

    if (!tempFile)
    {
        return Error("Failed to write header to temporary file");
    }

    for (const auto& packageName : cache.packageNames)
    {
        tempFile << packageName << "\n";
        if (!tempFile)
        {
            return Error("Failed to write package name to temporary file");
        }
    }

    tempFile.close();
    if (!tempFile)
    {
        return Error("Failed to close temporary file");
    }
    if (0 != std::rename(tempPath.c_str(), path.c_str()))
    {
        return Error("Failed to rename temporary file to target path: " + tempPath + "->" + path);
    }
    tempFileRemover.Deactivate();
    return 0;
}

Result<PackageCache> GetInstalledPackagesRpm(ContextInterface& context)
{
    PackageCache cache;
    cache.packageManager = "rpm";
    cache.lastUpdateTime = time(NULL);

    const std::string cmd = "rpm -qa --qf='%{NAME}\n'";

    auto rpmqa = context.ExecuteCommand(cmd);
    if (!rpmqa.HasValue())
    {
        return Error("Failed to execute rpm command");
    }

    std::istringstream stream(rpmqa.Value());
    std::string packageName;
    while (std::getline(stream, packageName))
    {
        if (!packageName.empty())
        {
            cache.packageNames.insert(packageName);
        }
    }
    return cache;
}

Result<PackageCache> GetInstalledPackagesDpkg(ContextInterface& context)
{
    PackageCache cache;
    cache.packageManager = "dpkg";
    cache.lastUpdateTime = time(NULL);

    const std::string cmd = "dpkg -l";

    auto dpkgl = context.ExecuteCommand(cmd);
    if (!dpkgl.HasValue())
    {
        return Error("Failed to execute dpkg command");
    }

    std::istringstream stream(dpkgl.Value());
    std::string line;
    bool headerSkipped = false;

    while (std::getline(stream, line))
    {
        if (!headerSkipped)
        {
            if (line.find("+++-") == 0)
            {
                headerSkipped = true;
            }
            continue;
        }

        if (line.size() >= 3 && line.substr(0, 3) == "ii ")
        {
            std::istringstream lineStream(line);
            std::string status, name;
            lineStream >> status >> name;
            if (!name.empty())
            {
                cache.packageNames.insert(name);
            }
        }
    }
    return cache;
}

Result<PackageCache> GetInstalledPackages(const std::string& packageManager, ContextInterface& context)
{
    if (packageManager == "rpm")
    {
        return GetInstalledPackagesRpm(context);
    }
    else if (packageManager == "dpkg")
    {
        return GetInstalledPackagesDpkg(context);
    }
    return Error("Unsupported package manager: " + packageManager);
}

} // namespace

namespace compliance
{

AUDIT_FN(PackageInstalled, "packageName:Package name:M", "packageManager:Package manager, autodetected by default::^(rpm|dpkg)$",
    "test_cachePath:Cache path")
{
    auto& logstream = context.GetLogstream();
    auto log = context.GetLogHandle();

    auto packageNameIt = args.find("packageName");
    if (packageNameIt == args.end())
    {
        context.GetLogstream() << "No package name provided";
        return Error("No package name provided");
    }
    auto packageName = std::move(packageNameIt->second);

    std::string packageManager;
    auto packageManagerIt = args.find("packageManager");
    if (packageManagerIt != args.end())
    {
        packageManager = std::move(packageManagerIt->second);
    }
    else
    {
        packageManager = DetectPackageManager(context);
        if (packageManager.empty())
        {
            logstream << "No package manager found";
            return Error("No package manager found");
        }
    }

    auto cachePathIt = args.find("test_cachePath");
    std::string cachePath = cachePathIt != args.end() ? std::move(cachePathIt->second) : PACKAGE_CACHE_PATH;

    PackageCache cache;
    auto cacheResult = LoadPackageCache(cachePath);
    bool cacheValid = true;
    bool cacheStale = false;
    if (cacheResult.HasValue())
    {
        cache = cacheResult.Value();
    }
    else
    {
        cacheValid = false;
        OsConfigLogInfo(log, "Failed to load package cache: %s", cacheResult.Error().message.c_str());
    }
    if (cacheValid && (cache.packageManager != packageManager))
    {
        cacheValid = false;
        OsConfigLogInfo(log, "Package manager mismatch: expected %s, found %s", cache.packageManager.c_str(), packageManager.c_str());
    }

    if (cacheValid)
    {
        auto cacheAge = time(NULL) - cache.lastUpdateTime;
        if (PACKAGELIST_STALE_TTL < cacheAge)
        {
            cacheValid = false;
            OsConfigLogInfo(log, "Package cache is stale over limit (%ld > %ld), cannot use", cacheAge, PACKAGELIST_STALE_TTL);
        }
        else if (PACKAGELIST_TTL < cacheAge)
        {
            cacheStale = true;
        }
    }

    if (!cacheValid || cacheStale)
    {
        cacheResult = GetInstalledPackages(packageManager, context);

        if (cacheResult.HasValue())
        {
            cache = cacheResult.Value();
            auto saveResult = SavePackageCache(cache, cachePath);
            if (saveResult.HasValue())
            {
                OsConfigLogInfo(log, "Saved package cache to %s", cachePath.c_str());
            }
            else
            {
                OsConfigLogError(log, "Failed to save package cache: %s", saveResult.Error().message.c_str());
            }
        }
        else
        {
            if (cacheStale)
            {
                OsConfigLogError(log, "Failed to get installed packages: %s, reusing stale cache", cacheResult.Error().message.c_str());
            }
            else
            {
                OsConfigLogError(log, "Failed to get installed packages: %s, cannot use cache", cacheResult.Error().message.c_str());
                return Error("Failed to get installed packages: " + cacheResult.Error().message);
            }
        }
    }

    if (cache.packageNames.find(packageName) != cache.packageNames.end())
    {
        logstream << "Package " << packageName << " is installed";
        return true;
    }
    else
    {
        logstream << "Package " << packageName << " is not installed";
        return false;
    }
}
} // namespace compliance
