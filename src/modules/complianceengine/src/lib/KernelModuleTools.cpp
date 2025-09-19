#include <CommonUtils.h>
#include <Evaluator.h>
#include <KernelModuleTools.h>
#include <Regex.h>
#include <dirent.h>
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

                if (baseName.find(target) == 0)
                {
                    moduleFound = true;
                    break;
                }
                if (baseName.find(overlayTarget) == 0)
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
        procModulesRegex = regex("^" + moduleName + "\\s+");
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

Result<Status> IsKernelModuleBlocked(std::string moduleName, IndicatorsTree& indicators, ContextInterface& context)
{
    Result<std::string> modprobeOutput = context.ExecuteCommand("modprobe --showconfig");
    if (modprobeOutput.HasValue())
    {

        regex modprobeBlacklistRegex;
        try
        {
            modprobeBlacklistRegex = regex("^blacklist\\s+" + moduleName + "$");
        }
        catch (std::exception& e)
        {
            return Error(e.what());
        }

        if (!MultilineRegexSearch(modprobeOutput.Value(), modprobeBlacklistRegex))
        {
            return indicators.NonCompliant("Module " + moduleName + " is not blacklisted in modprobe configuration");
        }

        regex modprobeInstallRegex;
        try
        {
            modprobeInstallRegex = regex("^install\\s+" + moduleName + "\\s+(/usr)?/bin/(true|false)");
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
    else
    {
        indicators.Compliant("Failed to execute modprobe: " + modprobeOutput.Error().message + ", ignoring modprobe output");
    }

    return indicators.Compliant("Module " + moduleName + " is disabled");
}

} // namespace ComplianceEngine
