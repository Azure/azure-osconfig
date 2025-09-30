// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <EnsureSysctl.h>
#include <Regex.h>
#include <StringTools.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace ComplianceEngine
{
Result<Status> AuditEnsureSysctl(const EnsureSysctlParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto log = context.GetLogHandle();
    std::string procfsLocation = "/proc/sys";

    auto sysctlPath = params.sysctlName;
    std::replace(sysctlPath.begin(), sysctlPath.end(), '.', '/');
    std::string procSysPath = procfsLocation + std::string("/") + sysctlPath;

    auto output = context.GetFileContents(procSysPath);
    if (!output.HasValue())
    {
        return output.Error();
    }
    std::string sysctlOutput = output.Value();

    // remove newline character
    if (*sysctlOutput.rbegin() == '\n')
    {
        sysctlOutput.erase(sysctlOutput.size() - 1);
    }

    if (regex_search(sysctlOutput, params.value) == false)
    {
        return indicators.NonCompliant("Expected '" + params.sysctlName + "' got '" + sysctlOutput + "' in runtime configuration");
    }
    else
    {
        indicators.Compliant("Correct value for '" + params.sysctlName + "' in runtime configuration");
    }

    // systemd-sysctl can be in different places on different OSes, we need to do some heuristics.
    std::string systemdSysctl;
    std::vector<std::string> systemdSysctlLocations = {"/lib/systemd/systemd-sysctl", "/usr/lib/systemd/systemd-sysctl"};
    const std::string argVersion = " --version";
    const std::string argCatConfig = " --cat-config";
    for (auto& cmd : systemdSysctlLocations)
    {
        auto result = context.ExecuteCommand(cmd + argVersion);
        if (result.HasValue())
        {
            systemdSysctl = cmd + argCatConfig;
            break;
        }
    }
    if (systemdSysctl.empty())
    {
        return Error("Cannot find systemd-sysctl command", ENOENT);
    }
    // systemd-sysctl shows all configs used by system that have sysctl's
    auto execResult = context.ExecuteCommand(systemdSysctl);
    if (!execResult.HasValue())
    {
        OsConfigLogError(log, "Failed to execute systemd-sysctl command");
        return execResult.Error();
    }

    std::istringstream configurations(execResult.Value());
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
            name = TrimWhiteSpaces(name);
            runSysctlValue = line.substr(eqPos + 1, line.size() - eqPos);
            runSysctlValue = TrimWhiteSpaces(runSysctlValue);

            if (name == params.sysctlName)
            {
                found = true;
                if (not regex_search(runSysctlValue, params.value))
                {
                    invalid = true;
                }
            }
        }
    }
    // we found a match with correct value
    if (found && !invalid)
    {
        return indicators.Compliant("Correct value for '" + params.sysctlName + "' in stored configuration");
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
        return indicators.NonCompliant("Expected '" + params.sysctlName + "' got '" + runSysctlValue + "' found in: '" + fileName + "'");
    }
    else
    {
        indicators.NonCompliant("Expected '" + params.sysctlName + "' not found in stored sysctl configuration");
    }

    auto ufwDefaults = context.GetFileContents("/etc/default/ufw");
    if (!ufwDefaults.HasValue())
    {
        return indicators.NonCompliant("Failed to read /etc/default/ufw: " + ufwDefaults.Error().message);
    }
    std::istringstream iss(ufwDefaults.Value());
    std::string sysctlFile;
    while (std::getline(iss, line))
    {
        if (line.find("IPT_SYSCTL=", 0) == 0)
        {
            sysctlFile = line.substr(std::string("IPT_SYSCTL=").size());
            break;
        }
    }
    if (sysctlFile.empty())
    {
        return indicators.NonCompliant("Failed to find IPT_SYSCTL in /etc/default/ufw");
    }
    auto sysctlContents = context.GetFileContents(TrimWhiteSpaces(sysctlFile));
    if (!sysctlContents.HasValue())
    {
        return indicators.NonCompliant("Failed to read ufw sysctl config file: " + sysctlContents.Error().message);
    }
    std::istringstream sysctlStream(sysctlContents.Value());
    while (std::getline(sysctlStream, line))
    {
        if (line.find(sysctlPath + "=", 0) == 0)
        {
            auto value = line.substr(params.sysctlName.size() + 1);
            if (regex_search(value, params.value))
            {
                return indicators.Compliant("Correct value for '" + params.sysctlName + "' in UFW configuration");
            }
            else
            {
                return indicators.NonCompliant("Expected '" + params.sysctlName + "', got '" + value + "' in UFW configuration");
            }
        }
    }

    return indicators.NonCompliant("Value not found in UFW configuration for '" + params.sysctlName + "'");
}
} // namespace ComplianceEngine
