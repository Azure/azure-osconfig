// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <iostream>
#include <string>

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

AUDIT_FN(EnsureKernelModuleUnavailable, "moduleName:Name of the kernel module:M")
{
    auto it = args.find("moduleName");
    if (it == args.end())
    {
        return Error("No module name provided");
    }
    auto moduleName = std::move(it->second);

    // For all of the Kernel versions in /lib/modules find all the files in the kernel (modules) directory.
    // TODO(wpk) replace this with std::filesystem when we can use C++17.
    std::string findCmd = "find /lib/modules/ -maxdepth 1 -mindepth 1 -type d | while read i; do find \"$i\"/kernel/ -type f; done";
    Result<std::string> findOutput = context.ExecuteCommand(findCmd);
    if (!findOutput.HasValue())
    {
        return Error("Failed to execute find command");
    }
    std::istringstream findStream(findOutput.Value());
    std::string line;
    bool moduleFound = false;
    while (std::getline(findStream, line))
    {
        auto pos = line.find_last_of("/");
        if (pos != std::string::npos)
        {
            line = line.substr(pos + 1);
        }
        // We use find because modules can have postfixes like .ko.gz.
        if (line.find(moduleName + ".ko") == 0)
        {
            moduleFound = true;
            break;
        }
        if (line.find(moduleName + "_overlay.ko") == 0)
        {
            moduleFound = true;
            moduleName = moduleName + "_overlay";
            break;
        }
    }

    if (!moduleFound)
    {
        return indicators.Compliant("Module " + moduleName + " not found");
    }

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
        return indicators.NonCompliant("Module " + moduleName + " is loaded");
    }

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
