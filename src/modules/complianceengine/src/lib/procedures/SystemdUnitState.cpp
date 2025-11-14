// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Regex.h>
#include <SystemdUnitState.h>
#include <Telemetry.h>
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
Result<Status> AuditSystemdUnitState(const SystemdUnitStateParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    struct systemdQueryParams
    {
        std::string argName;
        Optional<Pattern> pattern;
        systemdQueryParams(const char* name)
            : argName(name),
              pattern(Optional<Pattern>())
        {
        }
    };
    systemdQueryParams queryParams[] = {"ActiveState", "LoadState", "UnitFileState", "Unit"};
    bool argFound = false;
    auto log = context.GetLogHandle();
    std::string systemCtlCmd = "systemctl show ";

    auto setParamValue = [&](systemdQueryParams& param, Pattern pattern) {
        argFound = true;
        OsConfigLogDebug(log, "SystemdUnitState check unit name '%s' arg '%s'", params.unitName.c_str(), param.argName.c_str());
        param.pattern = std::move(pattern);
        systemCtlCmd += "-p " + param.argName + " ";
    };

    if (params.ActiveState.HasValue())
    {
        setParamValue(queryParams[0], params.ActiveState.Value());
    }
    if (params.LoadState.HasValue())
    {
        setParamValue(queryParams[1], params.LoadState.Value());
    }
    if (params.UnitFileState.HasValue())
    {
        setParamValue(queryParams[2], params.UnitFileState.Value());
    }
    if (params.Unit.HasValue())
    {
        setParamValue(queryParams[3], params.Unit.Value());
    }
    systemCtlCmd += params.unitName;

    if (!argFound)
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
        for (auto& param : queryParams)
        {
            if (!param.pattern.HasValue() || (name != param.argName))
            {
                continue;
            }
            if (!regex_match(value, param.pattern->GetRegex()))
            {
                // OsConfigLogDebug(log, "Failed to match systemctl unit name '%s' for name '%s' for pattern '%s'  for value '%s' ",
                // params.unitName.c_str(), name.c_str(), param.value.c_str(), value.c_str());
                OsConfigLogDebug(log, "Failed to match systemctl unit name '%s' for name '%s' for pattern '%s'", params.unitName.c_str(), name.c_str(),
                    value.c_str());
                return indicators.NonCompliant("Failed to match systemctl unit name '" + params.unitName + "' field '" + name + "' value '" + value +
                                               "' with pattern '" + param.pattern->GetPattern() + "'");
            }
            else
            {
                indicators.Compliant("Successfully matched systemctl unit name '" + params.unitName + "' field '" + name + "' value '" + value +
                                     "' with pattern '" + param.pattern->GetPattern() + "'");
            }
            matched = true;
        }
        if (matched == false)
        {
            OsConfigLogError(log, "Error match systemctl unit name '%s' state '%s' not matched any arguments", params.unitName.c_str(), name.c_str());
            OSConfigTelemetryStatusTrace("matched", EINVAL);
            return Status::NonCompliant;
        }
    }

    OsConfigLogDebug(log, "Success to match systemctl unit name '%s' for name all params ", params.unitName.c_str());
    return Status::Compliant;
}

} // namespace ComplianceEngine
