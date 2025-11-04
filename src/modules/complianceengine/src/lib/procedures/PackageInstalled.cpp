// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Bindings.h>
#include <CommonUtils.h>
#include <PackageInstalled.h>
#include <ProcedureMap.h>
#include <Result.h>
#include <cstdio>
#include <fstream>
#include <functional>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

namespace
{

using ComplianceEngine::ContextInterface;
using ComplianceEngine::Error;
using ComplianceEngine::PackageManagerType;
using ComplianceEngine::Result;

struct PackageCache
{
    time_t lastUpdateTime;
    PackageManagerType packageManager = PackageManagerType::Autodetect;
    std::map<std::string, std::string> packages;
};

static constexpr long PACKAGELIST_TTL = 3000L;        // just shy of an hours
static constexpr long PACKAGELIST_STALE_TTL = 12600L; // 3.5 hours

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

Result<PackageManagerType> DetectPackageManager(ContextInterface& context)
{
    auto output = context.ExecuteCommand("dpkg -l dpkg");
    if (output.HasValue())
    {
        return PackageManagerType::DPKG;
    }
    output = context.ExecuteCommand("rpm -q rpm");
    if (output.HasValue())
    {
        return PackageManagerType::RPM;
    }
    // For SLES 15
    output = context.ExecuteCommand("rpm -q rpm-ndb");
    if (output.HasValue())
    {
        return PackageManagerType::RPM;
    }

    return Error("No package manager found", ENOENT);
}

Result<PackageCache> LoadPackageCache(const std::string& path)
{
    PackageCache cache;
    const std::string pkgCacheHeader = "# PackageCache "; // "# PackageCache <packageManager>@<timestamp>\n"
    cache.lastUpdateTime = 0;
    cache.packageManager = PackageManagerType::Autodetect;
    cache.packages.clear();
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

    const auto packageMangerStr = header.substr(pkgCacheHeader.length(), separatorPos - pkgCacheHeader.length());
    if (packageMangerStr == "dpkg")
        cache.packageManager = PackageManagerType::DPKG;
    else if (packageMangerStr == "rpm")
        cache.packageManager = PackageManagerType::RPM;
    else
        return Error("Invalid package manager type");
    try
    {
        cache.lastUpdateTime = std::stol(header.substr(separatorPos + 1));
    }
    catch (const std::exception&)
    {
        return Error("Invalid timestamp in cache file header");
    }

    std::string line;
    while (std::getline(cacheFile, line))
    {
        if (!line.empty())
        {
            auto sepPos = line.find(' ');
            if (sepPos == std::string::npos)
            {
                continue; // Skip lines without a space
            }
            std::string packageName = line.substr(0, sepPos);
            std::string packageVersion = line.substr(sepPos + 1);
            cache.packages[packageName] = packageVersion;
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

    const std::string pkgCacheHeader = "# PackageCache " + std::to_string(cache.packageManager) + "@" + std::to_string(cache.lastUpdateTime) + "\n";
    tempFile << pkgCacheHeader;

    if (!tempFile)
    {
        return Error("Failed to write header to temporary file");
    }

    for (const auto& package : cache.packages)
    {
        tempFile << package.first << " " << package.second << "\n";
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
    cache.packageManager = PackageManagerType::RPM;
    cache.lastUpdateTime = time(NULL);

    const std::string cmd = "rpm -qa --qf='%{NAME} %{EVR}\n'";

    auto rpmqa = context.ExecuteCommand(cmd);
    if (!rpmqa.HasValue())
    {
        return Error("Failed to execute rpm command");
    }

    std::istringstream stream(rpmqa.Value());
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty())
        {
            continue;
        }
        auto separatorPos = line.find(' ');
        if (separatorPos == std::string::npos)
        {
            continue;
        }
        std::string packageName = line.substr(0, separatorPos);
        std::string packageVersion = line.substr(separatorPos + 1);
        cache.packages[packageName] = packageVersion;
    }
    return cache;
}

Result<PackageCache> GetInstalledPackagesDpkg(ContextInterface& context)
{
    PackageCache cache;
    cache.packageManager = PackageManagerType::DPKG;
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
            std::string status, name, version;
            lineStream >> status >> name >> version;
            if (!name.empty())
            {
                // some packages have arch after a colon (e.g. "foo:amd64"), get rid of it
                name = name.substr(0, name.find(':'));
                cache.packages[name] = version;
            }
        }
    }
    return cache;
}

int VersionCompare(const std::string& v1, const std::string& v2)
{
    auto evrSplit = [](const std::string& ver) {
        // Split version into epoch, version, and release parts.
        std::vector<std::string> evr;
        size_t pos = 0;
        size_t colonPos = ver.find_first_of(":", pos);
        if (colonPos != std::string::npos)
        {
            evr.push_back(ver.substr(pos, colonPos - pos));
            pos = colonPos + 1;
        }
        else
        {
            evr.push_back("0");
        }
        size_t dashPos = ver.find_last_of("-");
        if ((dashPos != std::string::npos) && (dashPos > pos))
        {
            evr.push_back(ver.substr(pos, dashPos - pos));
            pos = dashPos + 1;
            evr.push_back(ver.substr(pos));
        }
        else
        {
            evr.push_back(ver.substr(pos));
            evr.push_back("0");
        }
        return evr;
    };

    auto compareParts = [](const std::string& p1, const std::string& p2) {
        auto split = [](const std::string& ver) {
            std::vector<std::string> parts;
            for (size_t i = 0; i < ver.size();)
            {
                if (!isalnum(ver[i]))
                {
                    i++;
                    continue;
                }
                size_t j = i;
                if (isdigit(ver[j]))
                {
                    while ((j < ver.size()) && isdigit(ver[j]))
                    {
                        j++;
                    }
                }
                else
                {
                    while ((j < ver.size()) && !isdigit(ver[j]))
                    {
                        j++;
                    }
                }
                parts.push_back(ver.substr(i, j - i));
                i = j;
            }
            return parts;
        };
        auto parts1 = split(p1);
        auto parts2 = split(p2);
        size_t len = std::max(parts1.size(), parts2.size());
        for (size_t i = 0; i < len; i++)
        {
            if (i >= parts1.size())
            {
                return -1;
            }
            if (i >= parts2.size())
            {
                return 1;
            }
            // Split guarantees that parts are non-empty, so we can safely access parts1[i] and parts2[i].
            bool n1 = isdigit(parts1[i][0]);
            bool n2 = isdigit(parts2[i][0]);
            if (n1 && n2)
            {
                // Both parts are numeric, compare as integers.
                // Do it semi-manually to avoid overflows - strip leading zeroes, then compare lengths and then compare as strings.
                auto zeroPos1 = parts1[i].find_first_not_of("0");
                if (std::string::npos == zeroPos1)
                {
                    zeroPos1 = parts1[i].size() - 1;
                }
                auto zeroPos2 = parts2[i].find_first_not_of("0");
                if (std::string::npos == zeroPos2)
                {
                    zeroPos2 = parts2[i].size() - 1;
                }

                parts1[i] = parts1[i].substr(zeroPos1);
                parts2[i] = parts2[i].substr(zeroPos2);
                int cmp = parts1[i].size() - parts2[i].size();
                if (cmp < 0)
                {
                    return -1;
                }
                if (cmp > 0)
                {
                    return 1;
                }

                cmp = parts1[i].compare(parts2[i]);
                if (cmp < 0)
                {
                    return -1;
                }
                if (cmp > 0)
                {
                    return 1;
                }
            }
            else if (!n1 && !n2)
            {
                // Both parts are non-numeric, compare as strings.
                int cmp = parts1[i].compare(parts2[i]);
                if (cmp < 0)
                {
                    return -1;
                }
                if (cmp > 0)
                {
                    return 1;
                }
            }
            else
            {
                // One part is numeric, the other is not, the numeric part is more.
                if (n1)
                {
                    return 1;
                }
                else
                {
                    return -1;
                }
            }
        }
        return 0;
    };

    auto evr1 = evrSplit(v1);
    auto evr2 = evrSplit(v2);
    for (size_t i = 0; i < 3; i++)
    {
        int cmp = compareParts(evr1[i], evr2[i]);
        if (cmp < 0)
        {
            return -1;
        }
        if (cmp > 0)
        {
            return 1;
        }
    }
    return 0;
}

Result<PackageCache> GetInstalledPackages(PackageManagerType packageManager, ContextInterface& context)
{
    if (packageManager == PackageManagerType::RPM)
    {
        return GetInstalledPackagesRpm(context);
    }
    else if (packageManager == PackageManagerType::DPKG)
    {
        return GetInstalledPackagesDpkg(context);
    }
    return Error("Unsupported package manager: " + std::to_string(packageManager));
}

} // namespace

namespace ComplianceEngine
{
Result<Status> AuditPackageInstalled(const PackageInstalledParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.packageManager.HasValue());
    assert(params.test_cachePath.HasValue());
    auto log = context.GetLogHandle();

    PackageCache cache;
    auto cacheResult = LoadPackageCache(params.test_cachePath.Value());
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

    auto packageManager = params.packageManager.Value();
    if (cacheValid)
    {
        if (packageManager == PackageManagerType::Autodetect)
        {
            packageManager = cache.packageManager;
        }
        else if (cache.packageManager != packageManager)
        {
            cacheValid = false;
            OsConfigLogInfo(log, "Package manager mismatch: expected %s, found %s", std::to_string(cache.packageManager).c_str(),
                std::to_string(packageManager).c_str());
        }
    }
    else if (packageManager == PackageManagerType::Autodetect)
    {
        auto result = DetectPackageManager(context);
        if (!result.HasValue())
        {
            return result.Error();
        }

        packageManager = result.Value();
    }

    if (params.minPackageVersion.HasValue())
    {
        OsConfigLogInfo(log, "Checking if package %s is installed with minimum version %s using package manager %s", params.packageName.c_str(),
            params.minPackageVersion->c_str(), std::to_string(packageManager).c_str());
    }
    else
    {
        OsConfigLogInfo(log, "Checking if package %s is installed using package manager %s", params.packageName.c_str(), std::to_string(packageManager).c_str());
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
            auto saveResult = SavePackageCache(cache, params.test_cachePath.Value());
            if (saveResult.HasValue())
            {
                OsConfigLogInfo(log, "Saved package cache to %s", params.test_cachePath->c_str());
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

    auto packageIt = cache.packages.find(params.packageName);
    if (packageIt == cache.packages.end())
    {
        OsConfigLogInfo(log, "Package %s is not installed", params.packageName.c_str());
        return indicators.NonCompliant("Package " + params.packageName + " is not installed");
    }
    if (params.minPackageVersion.HasValue())
    {
        auto installedVersion = packageIt->second;
        if (VersionCompare(installedVersion, params.minPackageVersion.Value()) < 0)
        {
            OsConfigLogInfo(log, "Package %s is installed but version %s is less than minimum required version %s", params.packageName.c_str(),
                installedVersion.c_str(), params.minPackageVersion->c_str());
            return indicators.NonCompliant("Package " + params.packageName + " is installed but version " + installedVersion +
                                           " is less than minimum required version " + params.minPackageVersion.Value());
        }
        else
        {
            OsConfigLogInfo(log, "Package %s is installed with version %s, which meets or exceeds the minimum required version %s",
                params.packageName.c_str(), installedVersion.c_str(), params.minPackageVersion->c_str());
            return indicators.Compliant("Package " + params.packageName + " is installed with version " + installedVersion +
                                        ", which meets or exceeds the minimum required version " + params.minPackageVersion.Value());
        }
    }
    return indicators.Compliant("Package " + params.packageName + " is installed");
}
} // namespace ComplianceEngine
