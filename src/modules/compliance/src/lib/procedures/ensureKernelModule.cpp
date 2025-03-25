// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <iostream>
#include <regex>
#include <string>

namespace compliance
{

// TODO(wpk) std::regex::multiline is only supported in C++17.
static bool MultilineRegexSearch(const std::string& str, const std::regex& pattern)
{
    std::istringstream oss(str);
    std::string line;
    while (std::getline(oss, line))
    {
        if (std::regex_search(line, pattern))
        {
            return true;
        }
    }
    return false;
}

AUDIT_FN(ensureKernelModuleUnavailable)
{
    char* output = NULL;
    auto it = args.find("moduleName");
    if (it == args.end())
    {
        logstream << "No module name provided ";
        return Error("No module name provided");
    }
    auto moduleName = std::move(it->second);

    // For all of the Kernel versions in /lib/modules find all the files in the kernel (modules) directory.
    // TODO(wpk) replace this with std::filesystem when we can use C++17.
    std::string findCmd = "find /lib/modules/ -maxdepth 1 -mindepth 1 -type d | while read i; do find \"$i\"/kernel/ -type f; done";
    if ((0 != ExecuteCommand(NULL, findCmd.c_str(), false, false, 0, 0, &output, NULL, NULL)) || (output == NULL))
    {
        logstream << "Failed to execute find command ";
        return Error("Failed to execute find command");
    }
    std::string findOutput(output);
    free(output);
    output = NULL;

    if ((0 != ExecuteCommand(NULL, "lsmod", false, false, 0, 0, &output, NULL, NULL)) || (output == NULL))
    {
        logstream << "Failed to execute lsmod ";
        return Error("Failed to execute lsmod");
    }
    std::string lsmodOutput(output);
    free(output);
    output = NULL;

    if ((0 != ExecuteCommand(NULL, "modprobe --showconfig", false, false, 0, 0, &output, NULL, NULL)) || (output == NULL))
    {
        logstream << "Failed to execute modprobe ";
        return Error("Failed to execute modprobe");
    }
    std::string modprobeOutput(output);
    free(output);
    output = NULL;

    std::istringstream findStream(findOutput);
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
        logstream << "Module " << moduleName << " not found ";
        return true;
    }

    std::regex lsmodRegex;
    try
    {
        lsmodRegex = regex("^" + moduleName + "\\s+");
    }
    catch (std::exception& e)
    {
        return Error(e.what());
    }

    if (MultilineRegexSearch(lsmodOutput, lsmodRegex))
    {
        logstream << "Module " << moduleName << " is loaded ";
        return false;
    }

    std::regex modprobeBlacklistRegex;
    try
    {
        modprobeBlacklistRegex = regex("^blacklist\\s+" + moduleName + "$");
    }
    catch (std::exception& e)
    {
        return Error(e.what());
    }

    if (!MultilineRegexSearch(modprobeOutput, modprobeBlacklistRegex))
    {
        logstream << "Module " << moduleName << " is not blacklisted in modprobe configuration ";
        return false;
    }

    std::regex modprobeInstallRegex;
    try
    {
        modprobeInstallRegex = regex("^install\\s+" + moduleName + "\\s+(/usr)?/bin/(true|false)");
    }
    catch (std::exception& e)
    {
        return Error(e.what());
    }
    if (!MultilineRegexSearch(modprobeOutput, modprobeInstallRegex))
    {
        logstream << "Module " << moduleName << " is not masked in modprobe configuration ";
        return false;
    }

    logstream << "Module " << moduleName << " is disabled ";
    return true;
}

} // namespace compliance
