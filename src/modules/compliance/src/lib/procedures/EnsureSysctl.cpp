// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

namespace compliance
{
namespace
{
std::string TrimWhitesSpace(const std::string& str);
} // anonymous namespace

AUDIT_FN(EnsureSysctl, "sysctlName:Name of the sysctl:M:^([a-zA-Z0-9_]+[\\.a-zA-Z0-9_-]+)$", "value:Regex that the value of sysctl has to match:M",
    "test_procfs:Prefix for the /proc/sys from where sysctl will be read")
{
    auto log = context.GetLogHandle();
    char* output = NULL;
    std::string procfsLocation = "/proc/sys";
    std::string systemdSysctl = "/lib/systemd/systemd-sysctl";

    auto it = args.find("sysctlName");
    if (it == args.end())
    {
        return Error("Missing 'sysctlName' parameter", EINVAL);
    }
    auto sysctlName = std::move(it->second);

    it = args.find("value");
    if (it == args.end())
    {
        return Error("Missing 'value' parameter", EINVAL);
    }
    auto sysctlValue = std::move(it->second);

    it = args.find("test_procfs");
    if (it != args.end())
    {
        procfsLocation = std::move(it->second);
    }

    auto sysctlPath = sysctlName;
    std::replace(sysctlPath.begin(), sysctlPath.end(), '.', '/');
    std::string procSysPath = procfsLocation + std::string("/") + sysctlPath;

    output = LoadStringFromFile(procSysPath.c_str(), false, log);
    if (output == NULL)
    {
        return indicators.NonCompliant("Failed to load sysctl value from '" + procSysPath + "'");
    }
    std::string sysctlOutput(output);
    FREE_MEMORY(output);

    // remove newline caracter
    if (*sysctlOutput.rbegin() == '\n')
    {
        sysctlOutput.erase(sysctlOutput.size() - 1);
    }

    regex valueRegex;
    try
    {
        valueRegex = regex(sysctlValue);
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(log, "Regex error: %s", e.what());
        return Error("Failed to compile regex '" + sysctlValue + "' error: " + e.what());
    }

    if (regex_search(sysctlOutput, valueRegex) == false)
    {
        return indicators.NonCompliant("Expected '" + sysctlName + "' value: '" + sysctlValue + "' got '" + sysctlOutput + "'");
    }
    if (args.find("test_procfs") == args.end())
    {
        struct stat statbuf;
        if (0 != stat(systemdSysctl.c_str(), &statbuf))
        {
            systemdSysctl = std::string("/usr/lib/systemd/systemd-sysctl");
            if (0 != stat(systemdSysctl.c_str(), &statbuf))
            {
                OsConfigLogError(log, "Failed to locate systemd-sysctl command");
                return Error("Failed to locate systemd-sysctl command");
            }
        }
    }
    systemdSysctl += " --cat-config";
    // systemd-sysclt shows all configs used by system that have sysctl's
    if ((0 != ExecuteCommand(NULL, systemdSysctl.c_str(), false, false, 0, 0, &output, NULL, log)) || (output == NULL))
    {
        OsConfigLogError(log, "Failed to execute systemd-sysctl command");
        return Error("Failed to execute systemd-sysctl command");
    }
    std::string sysctlConfigs(output);
    FREE_MEMORY(output);

    std::istringstream configurations(sysctlConfigs);
    std::string line;
    std::string runSysctlValue;
    std::vector<std::string> sysctlConfigLines;

    // Reverse order of output, to check sysctls from most important till less important
    while (std::getline(configurations, line))
    {
        sysctlConfigLines.push_back(line);
    }

    bool found = false;
    bool invalid = false;
    auto lines = sysctlConfigLines.rbegin();
    // Iterate sysctlConfigLines backwards, when last value matches expected valueRegex
    // we are done.
    for (; !found && lines != sysctlConfigLines.rend(); ++lines)
    {
        line = *lines;
        size_t comment = line.find('#');
        if (comment != std::string::npos)
        {
            line = line.substr(0, comment);
        }
        if (line == "")
        {
            continue;
        }

        // TODO(kkanas) consider using regex_match when we have all platforms with up to date support
        auto nameValuePattern = regex("\\s*([a-zA-Z0-9_]+[\\.a-zA-Z0-9_-]+)\\s*=\\s*(.*)");
        if (regex_search(line, nameValuePattern))
        {
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos)
            {
                continue;
            }

            std::string name = line.substr(0, eqPos);
            name = TrimWhitesSpace(name);
            runSysctlValue = line.substr(eqPos + 1, line.size() - eqPos);
            runSysctlValue = TrimWhitesSpace(runSysctlValue);

            if (name == sysctlName)
            {
                found = true;
                if (not regex_search(runSysctlValue, valueRegex))
                {
                    invalid = true;
                }
            }
        }
    }
    // we found a match with correct value
    if (found && !invalid)
    {
        return indicators.Compliant("Correct value for '" + sysctlName + "': '" + sysctlValue + "'");
    }

    // lines are iterated backwards so filename is before last value marked by lines
    std::string fileName;
    for (; lines != sysctlConfigLines.rend(); ++lines)
    {
        line = *lines;
        // TODO(kkanas) consider using regex_match when we have all platforms with up to date support
        regex fileNamePattern("^\\s*#\\s*(\\/.*\\.conf)$");
        if (regex_search(line, fileNamePattern))
        {
            size_t comment = line.find('#');
            if (comment == std::string::npos)
            {
                continue;
            }
            size_t slash = line.find_first_of('/', comment + 1);
            if (slash == std::string::npos)
            {
                continue;
            }
            fileName = line.substr(slash, line.size() - slash + 1);
            break;
        }
    }

    if (found && invalid)
    {
        return indicators.NonCompliant("Expected '" + sysctlName + "' value: '" + sysctlValue + "' got '" + runSysctlValue + "' found in: '" +
                                       fileName + "'");
    }
    return indicators.NonCompliant("Expected '" + sysctlName + "' value: '" + sysctlValue + " not found in system");
}

namespace
{
std::string TrimWhitesSpace(const std::string& str)
{
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    if (start < end)
    {
        return std::string(start, end);
    }
    return std::string();
}
} // anonymous namespace
} // namespace compliance
