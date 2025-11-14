// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <SystemdConfig.h>
#include <Telemetry.h>
#include <algorithm>
#include <fts.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using ComplianceEngine::Optional;

namespace ComplianceEngine
{

namespace
{
typedef std::map<std::string, std::pair<std::string, std::string>> SystemdConfigMap_t;

Result<bool> GetSystemdConfig(SystemdConfigMap_t& config, const std::string& filename, ContextInterface& context)
{
    auto result = context.ExecuteCommand("/usr/bin/systemd-analyze cat-config " + filename);
    if (!result.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to execute systemd-analyze command: %s", result.Error().message.c_str());
        OSConfigTelemetryStatusTrace("ExecuteCommand", result.Error().code);
        return result.Error();
    }
    std::istringstream stream(result.Value());
    std::string line;
    std::string currentConfig = "<UNKNOWN>";
    while (std::getline(stream, line))
    {
        if (line.empty())
        {
            continue;
        }
        if ('#' == line[0])
        {
            if ((line.size() > strlen("# .conf")) && ('#' == line[0]) && (".conf" == line.substr(line.size() - strlen(".conf"))))
            {
                currentConfig = line.substr(2);
            }
            continue;
        }
        size_t eqSign = line.find('=');
        if (eqSign == std::string::npos)
        {
            OsConfigLogError(context.GetLogHandle(), "Invalid line in systemd config: %s", line.c_str());
            OSConfigTelemetryStatusTrace("getline", EINVAL);
            continue;
        }
        std::string key = line.substr(0, eqSign);
        std::string value = line.substr(eqSign + 1);
        config[key] = std::make_pair(value, currentConfig);
    }
    return true;
}
} // namespace

Result<Status> AuditSystemdParameter(const SystemdParameterParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto log = context.GetLogHandle();

    if (!params.dir.HasValue() && !params.file.HasValue())
    {
        OsConfigLogError(log, "Error: SystemdParameter: neither 'file' nor 'dir' argument is provided");
        OSConfigTelemetryStatusTrace("dir.empty && filename.empty", EINVAL);
        return Error("Neither 'file' nor 'dir' argument is provided");
    }
    if (params.dir.HasValue() && params.file.HasValue())
    {
        OsConfigLogError(log, "Error: SystemdParameter: both 'file' and 'dir' arguments are provided, only one is allowed");
        OSConfigTelemetryStatusTrace("one dir or file only", EINVAL);
        return Error("Both 'file' and 'dir' arguments are provided, only one is allowed");
    }

    SystemdConfigMap_t config;
    if (params.file.HasValue())
    {
        OsConfigLogDebug(log, "Getting systemd config for file '%s'", params.file->c_str());
        auto result = GetSystemdConfig(config, params.file.Value(), context);
        if (!result.HasValue())
        {
            OsConfigLogError(log, "Failed to get systemd config for file '%s' - %s", params.file->c_str(), result.Error().message.c_str());
            OSConfigTelemetryStatusTrace("GetSystemdConfig", result.Error().code);
            return result.Error();
        }
    }
    else
    {
        OsConfigLogDebug(log, "Getting systemd config for directory '%s'", params.dir->c_str());
        bool anySuccess = false;
        char* paths[] = {const_cast<char*>(params.dir->c_str()), nullptr};
        FTS* file_system = fts_open(paths, FTS_NOCHDIR | FTS_PHYSICAL, nullptr);
        if (!file_system)
        {
            OsConfigLogError(log, "Failed to open directory '%s' with fts", params.dir->c_str());
            return Error("Failed to open directory '" + params.dir.Value() + "'");
        }

        FTSENT* node = nullptr;
        while ((node = fts_read(file_system)) != nullptr)
        {
            if (node->fts_info == FTS_F)
            {
                std::string filePath = node->fts_path;
                if (filePath.size() >= strlen(".conf") && filePath.substr(filePath.size() - 5) == ".conf")
                {
                    OsConfigLogDebug(log, "Getting systemd config for file '%s' in directory '%s'", filePath.c_str(), params.dir->c_str());
                    auto result = GetSystemdConfig(config, filePath, context);
                    if (!result.HasValue())
                    {
                        OsConfigLogError(log, "Failed to get systemd config for file '%s' - %s", filePath.c_str(), result.Error().message.c_str());
                        OSConfigTelemetryStatusTrace("GetSystemdConfig", result.Error().code);
                    }
                    else
                    {
                        anySuccess = true;
                        OsConfigLogDebug(log, "Successfully got systemd config for file '%s'", filePath.c_str());
                    }
                }
            }
        }
        fts_close(file_system);
        if (!anySuccess)
        {
            OsConfigLogError(log, "No valid systemd config files found in directory '%s'", params.dir->c_str());
            OSConfigTelemetryStatusTrace("fts_close", errno ? errno : EINVAL);
            return Error("No valid systemd config files found in directory '" + params.dir.Value() + "'");
        }
    }

    auto paramIt = config.find(params.parameter);
    if (paramIt == config.end())
    {
        OsConfigLogInfo(log, "Parameter '%s' not found", params.parameter.c_str());
        return indicators.NonCompliant("Parameter '" + params.parameter + "' not found");
    }

    OsConfigLogDebug(log, "Parameter '%s' found in file '%s' with value '%s'", params.parameter.c_str(), paramIt->second.second.c_str(),
        paramIt->second.first.c_str());
    if (!regex_match(paramIt->second.first, params.valueRegex))
    {
        OsConfigLogInfo(log, "Parameter '%s' in file '%s' does not match regex", params.parameter.c_str(), paramIt->second.second.c_str());
        return indicators.NonCompliant("Parameter '" + params.parameter + "' value '" + paramIt->second.first + "' in file '" + paramIt->second.second +
                                       "' does not match regex");
    }
    else
    {
        return indicators.Compliant("Parameter '" + params.parameter + "' found in file '" + paramIt->second.second + "' with value '" +
                                    paramIt->second.first + "'");
    }
}

} // namespace ComplianceEngine
