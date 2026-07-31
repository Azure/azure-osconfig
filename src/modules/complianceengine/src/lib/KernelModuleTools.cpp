#include <CommonUtils.h>
#include <Evaluator.h>
#include <KernelModuleTools.h>
#include <Regex.h>
#include <Telemetry.h>
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <fts.h>
#include <iostream>
#include <string>
#include <sys/stat.h>

namespace ComplianceEngine
{
// TODO(wpk) std::regex::multiline is only supported in C++17.
static bool MultilineRegexSearch(const std::string& str, const regex& pattern)
{
    std::istringstream oss(str);
    std::string line;
    while (std::getline(oss, line))
    {
        if (regex_search(line, pattern))
        {
            return true;
        }
    }
    return false;
}
// Searches /lib/modules for moduleName, including overlay.ko modules and returns true if found
Result<bool> SearchFilesystemForModuleName(std::string& moduleName, ContextInterface& context)
{
    std::string modulesDirPath = context.GetSpecialFilePath("/lib/modules");
    DIR* modulesDir = opendir(modulesDirPath.c_str());
    if (!modulesDir)
    {
        return Error("Failed to open /lib/modules directory");
    }
    auto modulesDirDeleter = std::unique_ptr<DIR, int (*)(DIR*)>(modulesDir, closedir);

    std::vector<std::string> kernelModuleFileBasenames; // Collected file basenames (no directory prefix)
    bool moduleFound = false;

    struct dirent* entry = nullptr;
    while ((entry = readdir(modulesDir)) != nullptr && !moduleFound)
    {
        if (entry->d_type != DT_DIR)
        {
            continue;
        }

        std::string modulesVersionDir = modulesDirPath + "/" + entry->d_name + "/kernel";
        struct stat st;
        if (stat(modulesVersionDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
        {
            continue;
        }

        char* paths[] = {const_cast<char*>(modulesVersionDir.c_str()), nullptr};
        // Use FTS_PHYSICAL to avoid following symlinks; omit FTS_NOCHDIR for portability.
        FTS* fts = fts_open(paths, FTS_PHYSICAL, nullptr);
        if (!fts)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to open %s - errno %d", modulesVersionDir.c_str(), errno);
            OSConfigTelemetryStatusTrace("fts_open", errno);
            continue;
        }
        auto ftspDeleter = std::unique_ptr<FTS, int (*)(FTS*)>(fts, fts_close);

        FTSENT* node = nullptr;
        while (!moduleFound && (node = fts_read(fts)) != nullptr)
        {
            if (node->fts_info == FTS_F)
            {
                std::string baseName = node->fts_name;

                std::string target = moduleName + ".ko";
                std::string overlayTarget = moduleName + "_overlay.ko";

                // Linux kernel uses underscores in .ko filenames, but module names may use dashes
                // (e.g., "firewire-core" module is stored as "firewire_core.ko")
                std::string targetUnderscore = target;
                std::replace(targetUnderscore.begin(), targetUnderscore.end(), '-', '_');
                std::string overlayTargetUnderscore = overlayTarget;
                std::replace(overlayTargetUnderscore.begin(), overlayTargetUnderscore.end(), '-', '_');

                if (baseName.find(target) == 0 || baseName.find(targetUnderscore) == 0)
                {
                    moduleFound = true;
                    break;
                }
                if (baseName.find(overlayTarget) == 0 || baseName.find(overlayTargetUnderscore) == 0)
                {
                    moduleFound = true;
                    moduleName += "_overlay"; // preserve original behavior of tracking overlay variant
                    break;
                }
            }
        }
    }
    return moduleFound;
}

static std::string UnderscoreForRegex(std::string input)
{
    std::string result = input;
    size_t pos = 0;
    while ((pos = result.find('-', pos)) != std::string::npos)
    {
        result.replace(pos, 1, "[-_]");
        pos += 4;
    }
    return result;
}

Result<bool> IsKernelModuleLoaded(std::string moduleName, ContextInterface& context)
{
    Result<std::string> procModules = context.GetFileContents("/proc/modules");

    if (!procModules.HasValue())
    {
        return procModules.Error();
    }
    regex procModulesRegex;
    try
    {
        procModulesRegex = regex("^" + UnderscoreForRegex(moduleName) + "\\s+");
    }
    catch (regex_error& e)
    {
        return Error(e.what());
    }

    if (MultilineRegexSearch(procModules.Value(), procModulesRegex))
    {
        return true;
    }
    return false;
}

// Reads the currently-running kernel release (equivalent to `uname -r`) from
// /proc/sys/kernel/osrelease. Returns an empty string if it cannot be read.
static std::string GetRunningKernelRelease(ContextInterface& context)
{
    std::ifstream ifs(context.GetSpecialFilePath("/proc/sys/kernel/osrelease"));
    std::string release;
    std::getline(ifs, release);
    while (!release.empty() && (release.back() == '\n' || release.back() == '\r' || release.back() == ' ' || release.back() == '\t'))
    {
        release.pop_back();
    }
    return release;
}

// Returns true if the module object file is present in the currently-running
// kernel's module directory (/lib/modules/$(uname -r)/kernel).
//
// CIS only requires the "install <module> /bin/false" masking when the module is
// loadable in the running kernel. When the module exists only in a non-running
// installed kernel (or not at all in the running kernel), deny-listing alone is
// sufficient, so the mask must not be required in that case.
Result<bool> IsModuleAvailableInRunningKernel(const std::string& moduleName, ContextInterface& context)
{
    std::string release = GetRunningKernelRelease(context);
    if (release.empty())
    {
        // Unable to determine the running kernel; be conservative and assume the module
        // may be loadable so the stricter mask requirement still applies.
        return true;
    }

    std::string kernelDirPath = context.GetSpecialFilePath("/lib/modules") + "/" + release + "/kernel";
    struct stat st;
    if (stat(kernelDirPath.c_str(), &st) != 0)
    {
        if (errno == ENOENT)
        {
            return false;
        }
        OsConfigLogError(context.GetLogHandle(), "Failed to stat %s - errno %d", kernelDirPath.c_str(), errno);
        OSConfigTelemetryStatusTrace("stat", errno);
        return true;
    }
    if (!S_ISDIR(st.st_mode))
    {
        return false;
    }

    char* paths[] = {const_cast<char*>(kernelDirPath.c_str()), nullptr};
    FTS* fts = fts_open(paths, FTS_PHYSICAL, nullptr);
    if (!fts)
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to open %s - errno %d", kernelDirPath.c_str(), errno);
        OSConfigTelemetryStatusTrace("fts_open", errno);
        return false;
    }
    auto ftsDeleter = std::unique_ptr<FTS, int (*)(FTS*)>(fts, fts_close);

    std::string target = moduleName + ".ko";
    std::string overlayTarget = moduleName + "_overlay.ko";
    std::string targetUnderscore = target;
    std::replace(targetUnderscore.begin(), targetUnderscore.end(), '-', '_');
    std::string overlayTargetUnderscore = overlayTarget;
    std::replace(overlayTargetUnderscore.begin(), overlayTargetUnderscore.end(), '-', '_');

    FTSENT* node = nullptr;
    while ((node = fts_read(fts)) != nullptr)
    {
        if (node->fts_info != FTS_F)
        {
            continue;
        }
        std::string baseName = node->fts_name;
        if (baseName.find(target) == 0 || baseName.find(targetUnderscore) == 0 || baseName.find(overlayTarget) == 0 || baseName.find(overlayTargetUnderscore) == 0)
        {
            return true;
        }
    }
    return false;
}

Result<Status> IsKernelModuleBlocked(std::string moduleName, bool requireMask, IndicatorsTree& indicators, ContextInterface& context)
{
    Result<std::string> modprobeOutput = context.ExecuteCommand("modprobe --showconfig");
    if (modprobeOutput.HasValue())
    {

        regex modprobeBlacklistRegex;
        try
        {
            modprobeBlacklistRegex = regex("^blacklist\\s+" + UnderscoreForRegex(moduleName) + "$");
        }
        catch (std::exception& e)
        {
            return Error(e.what());
        }

        if (!MultilineRegexSearch(modprobeOutput.Value(), modprobeBlacklistRegex))
        {
            return indicators.NonCompliant("Module " + moduleName + " is not blacklisted in modprobe configuration");
        }

        if (requireMask)
        {
            regex modprobeInstallRegex;
            try
            {
                modprobeInstallRegex = regex("^install\\s+" + UnderscoreForRegex(moduleName) + "\\s+(/usr)?/bin/(true|false)");
            }
            catch (std::exception& e)
            {
                return Error(e.what());
            }
            if (!MultilineRegexSearch(modprobeOutput.Value(), modprobeInstallRegex))
            {
                return indicators.NonCompliant("Module " + moduleName + " is not masked in modprobe configuration");
            }
        }
    }
    else
    {
        indicators.Compliant("Failed to execute modprobe: " + modprobeOutput.Error().message + ", ignoring modprobe output");
    }

    return indicators.Compliant("Module " + moduleName + " is disabled");
}

} // namespace ComplianceEngine
