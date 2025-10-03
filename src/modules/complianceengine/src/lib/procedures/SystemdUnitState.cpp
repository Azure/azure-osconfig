// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Internal.h>
#include <Regex.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using ComplianceEngine::Optional;

namespace ComplianceEngine
{

// Documentation for dbus {ActiveState, LoadState, UnitFileState} possible values and meaning
// https://www.freedesktop.org/wiki/Software/systemd/dbus/
// man systemd.timer /Unit=
// https://www.freedesktop.org/software/systemd/man/latest/systemd.timer.html
// Unit=  # The unit to activate when this timer elapses.
AUDIT_FN(SystemdUnitState, "unitName:Name of the systemd unit:M", "ActiveState:value of systemd ActiveState of unitName to match",
    "LoadState:value of systemd LoadState of unitName to match", "UnitFileState:value of systemd UnitFileState of unitName to match",
    "Unit:value of systemd property Unit, used in systemd.timer, name of unit to run when timer elapses ")
{
    struct systemdQueryParams
    {
        std::string argName;
        std::string value;
        Optional<regex> valueRegex;
        systemdQueryParams(std::string name)
            : argName(name),
              value("")
        {
        }
    };
    systemdQueryParams params[] = {
        systemdQueryParams("ActiveState"), systemdQueryParams("LoadState"), systemdQueryParams("UnitFileState"), systemdQueryParams("Unit")};
    bool argFound = false;
    auto log = context.GetLogHandle();
    std::string systemCtlCmd = "systemctl show ";

    auto it = args.find("unitName");
    if (it == args.end())
    {
        OsConfigLogError(log, "Error: EnsureSystemdUnit: missing 'unitName' parameter ");
        OSConfigTelemetryStatusTrace("unitName", EINVAL);
        return Error("Missing 'unitName' parameter");
    }
    auto unitName = std::move(it->second);

    for (auto& param : params)
    {
        it = args.find(param.argName);
        if (it == args.end())
        {
            continue;
        }
        argFound = true;
        param.value = it->second;
        OsConfigLogDebug(log, "SystemdUnitState check unit name '%s' arg '%s' value '%s'", unitName.c_str(), param.argName.c_str(), param.value.c_str());
        try
        {
            param.valueRegex = regex(param.value);
            systemCtlCmd += "-p " + param.argName + " ";
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(log, "Regex error: %s", e.what());
            OSConfigTelemetryStatusTrace("regex", EINVAL);
            return Error("Failed to compile regex '" + param.value + "' error: " + e.what());
        }
    }
    systemCtlCmd += unitName;

    if (argFound == false)
    {
        OsConfigLogError(log, "Error: EnsureSystemdUnit: none of 'activeState loadState UnitFileState' parameters are present");
        OSConfigTelemetryStatusTrace("argFound", EINVAL);
        return Error("None of 'activeState loadState UnitFileState' parameters are present");
    }

    Result<std::string> systemCtlOutput = context.ExecuteCommand(systemCtlCmd);
    if (!systemCtlOutput.HasValue())
    {
        OsConfigLogError(log, "Failed to execute systemctl command '%s': %s (code: %d)", systemCtlCmd.c_str(), systemCtlOutput.Error().message.c_str(),
            systemCtlOutput.Error().code);
        OSConfigTelemetryStatusTrace("ExecuteCommand", systemCtlOutput.Error().code);
        return indicators.NonCompliant("Failed to execute systemctl command " + systemCtlOutput.Error().message);
    }
    std::string line;
    std::istringstream sysctlValues(systemCtlOutput.Value());

    while (std::getline(sysctlValues, line))
    {
        size_t eqSign = line.find('=');
        if (eqSign == std::string::npos)
        {
            OsConfigLogError(log, "Error: EnsureSystemdUnit: invalid sysctl output, missing '=' sing in %s", line.c_str());
            OSConfigTelemetryStatusTrace("find", EINVAL);
            return indicators.NonCompliant("invalid sysctl output, missing '='  in  output '" + line + "'");
        }
        auto name = line.substr(0, eqSign);
        auto value = line.substr(eqSign + 1);
        bool matched = false;
        for (auto& param : params)
        {
            if (!param.valueRegex.HasValue() || (name != param.argName))
            {
                continue;
            }
            if (!regex_match(value, param.valueRegex.Value()))
            {
                OsConfigLogDebug(log, "Failed to match systemctl unit name '%s' for name '%s' for pattern '%s'  for value '%s' ", unitName.c_str(),
                    name.c_str(), param.value.c_str(), value.c_str());
                return indicators.NonCompliant("Failed to match systemctl unit name '" + unitName + "' field '" + name + "' value '" + value +
                                               "' with pattern '" + param.value + "'");
            }
            else
            {
                indicators.Compliant("Successfully matched systemctl unit name '" + unitName + "' field '" + name + "' value '" + value +
                                     "' with pattern '" + param.value + "'");
            }
            matched = true;
        }
        if (matched == false)
        {
            OsConfigLogError(log, "Error match systemctl unit name '%s' state '%s' not matched any arguments", unitName.c_str(), name.c_str());
            OSConfigTelemetryStatusTrace("matched", EINVAL);
            return Status::NonCompliant;
        }
    }

    OsConfigLogDebug(log, "Success to match systemctl unit name '%s' for name all params ", unitName.c_str());
    return Status::Compliant;
}

} // namespace ComplianceEngine
