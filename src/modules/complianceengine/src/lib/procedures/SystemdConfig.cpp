// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
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
            continue;
        }
        std::string key = line.substr(0, eqSign);
        std::string value = line.substr(eqSign + 1);
        config[key] = std::make_pair(value, currentConfig);
    }
    return true;
}
} // namespace

AUDIT_FN(SystemdParameter, "parameter:Parameter name:M", "valueRegex:Regex for the value:M", "file:Config filename",
    "dir:Directory to search for config files")
{
    auto log = context.GetLogHandle();
    auto it = args.find("parameter");
    if (it == args.end())
    {
        OsConfigLogError(log, "Error: SystemdParameter: missing 'parameter' argument");
        return Error("Missing 'parameter' argument");
    }
    std::string parameter = it->second;

    it = args.find("valueRegex");
    if (it == args.end())
    {
        OsConfigLogError(log, "Error: SystemdParameter: missing 'valueRegex' argument");
        return Error("Missing 'valueRegex' argument");
    }
    std::string valueRegex = it->second;
    regex valueRegexV;
    try
    {
        valueRegexV = regex(valueRegex);
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(log, "Failed to compile regex '%s': %s", valueRegex.c_str(), e.what());
        return Error("Failed to compile regex '" + valueRegex + "': " + e.what());
    }

    it = args.find("file");
    std::string filename;
    if (it != args.end())
    {
        filename = std::move(it->second);
    }

    it = args.find("dir");
    std::string dir;
    if (it != args.end())
    {
        dir = std::move(it->second);
    }

    if (dir.empty() && filename.empty())
    {
        OsConfigLogError(log, "Error: SystemdParameter: neither 'file' nor 'dir' argument is provided");
        return Error("Neither 'file' nor 'dir' argument is provided");
    }
    if (!dir.empty() && !filename.empty())
    {
        OsConfigLogError(log, "Error: SystemdParameter: both 'file' and 'dir' arguments are provided, only one is allowed");
        return Error("Both 'file' and 'dir' arguments are provided, only one is allowed");
    }

    SystemdConfigMap_t config;
    if (!filename.empty())
    {
        OsConfigLogDebug(log, "Getting systemd config for file '%s'", filename.c_str());
        auto result = GetSystemdConfig(config, filename, context);
        if (!result.HasValue())
        {
            OsConfigLogError(log, "Failed to get systemd config for file '%s' - %s", filename.c_str(), result.Error().message.c_str());
            return result.Error();
        }
    }
    else
    {
        OsConfigLogDebug(log, "Getting systemd config for directory '%s'", dir.c_str());
        bool anySuccess = false;
        char* paths[] = {const_cast<char*>(dir.c_str()), nullptr};
        FTS* file_system = fts_open(paths, FTS_NOCHDIR | FTS_PHYSICAL, nullptr);
        if (!file_system)
        {
            OsConfigLogError(log, "Failed to open directory '%s' with fts", dir.c_str());
            return Error("Failed to open directory '" + dir + "'");
        }

        FTSENT* node = nullptr;
        while ((node = fts_read(file_system)) != nullptr)
        {
            if (node->fts_info == FTS_F)
            {
                std::string filePath = node->fts_path;
                if (filePath.size() >= strlen(".conf") && filePath.substr(filePath.size() - 5) == ".conf")
                {
                    OsConfigLogDebug(log, "Getting systemd config for file '%s' in directory '%s'", filePath.c_str(), dir.c_str());
                    auto result = GetSystemdConfig(config, filePath, context);
                    if (!result.HasValue())
                    {
                        OsConfigLogError(log, "Failed to get systemd config for file '%s' - %s", filePath.c_str(), result.Error().message.c_str());
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
            OsConfigLogError(log, "No valid systemd config files found in directory '%s'", dir.c_str());
            return Error("No valid systemd config files found in directory '" + dir + "'");
        }
    }

    auto paramIt = config.find(parameter);
    if (paramIt == config.end())
    {
        OsConfigLogInfo(log, "Parameter '%s' not found in file '%s'", parameter.c_str(), filename.c_str());
        return indicators.NonCompliant("Parameter '" + parameter + "' not found in file '" + filename + "'");
    }

    OsConfigLogDebug(log, "Parameter '%s' found in file '%s' with value '%s'", parameter.c_str(), paramIt->second.second.c_str(), paramIt->second.first.c_str());
    if (!regex_match(paramIt->second.first, valueRegexV))
    {
        OsConfigLogInfo(log, "Parameter '%s' in file '%s' does not match regex '%s'", parameter.c_str(), paramIt->second.second.c_str(), valueRegex.c_str());
        return indicators.NonCompliant("Parameter '" + parameter + "' value '" + paramIt->second.first + "' in file '" + paramIt->second.second +
                                       "' does not match regex '" + valueRegex + "'");
    }
    else
    {
        return indicators.Compliant("Parameter '" + parameter + "' found in file '" + paramIt->second.second + "' with value '" +
                                    paramIt->second.first + "'");
    }
}

} // namespace ComplianceEngine
