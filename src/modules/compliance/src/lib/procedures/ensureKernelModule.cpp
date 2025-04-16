// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <iostream>
#include <string>

namespace compliance
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
        context.GetLogstream() << "No module name provided ";
        return Error("No module name provided");
    }
    auto moduleName = std::move(it->second);

    // For all of the Kernel versions in /lib/modules find all the files in the kernel (modules) directory.
    // TODO(wpk) replace this with std::filesystem when we can use C++17.
    std::string findCmd = "find /lib/modules/ -maxdepth 1 -mindepth 1 -type d | while read i; do find \"$i\"/kernel/ -type f; done";
    Result<std::string> findOutput = context.ExecuteCommand(findCmd);
    if (!findOutput.HasValue())
    {
        context.GetLogstream() << "find /lib/modules: " << findOutput.Error().message;
        return findOutput.Error();
    }

    Result<std::string> procModules = context.GetFileContents("/proc/modules");
    if (!procModules.HasValue())
    {
        context.GetLogstream() << "procModules: " << procModules.Error();
        return procModules.Error();
    }

    Result<std::string> modprobeOutput = context.ExecuteCommand("modprobe --showconfig");
    if (!modprobeOutput.HasValue())
    {
        context.GetLogstream() << "modprobe --showconfig: " << modprobeOutput.Error().message;
        return modprobeOutput.Error();
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
        context.GetLogstream() << "Module " << moduleName << " not found ";
        return true;
    }

    regex procModulesRegex;
    try
    {
        procModulesRegex = regex("^" + moduleName + "\\s+");
    }
    catch (std::exception& e)
    {
        return Error(e.what());
    }

    if (MultilineRegexSearch(procModules.Value(), procModulesRegex))
    {
        context.GetLogstream() << "Module " << moduleName << " is loaded ";
        return false;
    }

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
        context.GetLogstream() << "Module " << moduleName << " is not blacklisted in modprobe configuration ";
        return false;
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
        context.GetLogstream() << "Module " << moduleName << " is not masked in modprobe configuration ";
        return false;
    }

    context.GetLogstream() << "Module " << moduleName << " is disabled ";
    return true;
}

} // namespace compliance
