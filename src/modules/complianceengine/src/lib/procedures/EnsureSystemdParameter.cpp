// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <EnsureSystemdParameter.h>
#include <SystemdConfig.h>
#include <algorithm>
#include <fts.h>
#include <iostream>
#include <string>
#include <sys/types.h>

using ComplianceEngine::Optional;

namespace ComplianceEngine
{
Result<Status> AuditSystemdParameter(const SystemdParameterParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto log = context.GetLogHandle();

    if (!params.dir.HasValue() && !params.file.HasValue())
    {
        OsConfigLogError(log, "Error: SystemdParameter: neither 'file' nor 'dir' argument is provided");
        return Error("Neither 'file' nor 'dir' argument is provided");
    }
    if (params.dir.HasValue() && params.file.HasValue())
    {
        OsConfigLogError(log, "Error: SystemdParameter: both 'file' and 'dir' arguments are provided, only one is allowed");
        return Error("Both 'file' and 'dir' arguments are provided, only one is allowed");
    }

    SystemdConfig config;
    if (params.file.HasValue())
    {
        OsConfigLogDebug(log, "Getting systemd config for file '%s'", params.file->c_str());
        auto result = GetSystemdConfig(params.file.Value(), context);
        if (!result.HasValue())
        {
            OsConfigLogError(log, "Failed to get systemd config for file '%s' - %s", params.file->c_str(), result.Error().message.c_str());
            return result.Error();
        }

        config = std::move(result.Value());
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
                    auto result = GetSystemdConfig(filePath, context);
                    if (!result.HasValue())
                    {
                        OsConfigLogError(log, "Failed to get systemd config for file '%s' - %s", filePath.c_str(), result.Error().message.c_str());
                    }
                    else
                    {
                        anySuccess = true;
                        OsConfigLogDebug(log, "Successfully got systemd config for file '%s'", filePath.c_str());
                        config.Merge(std::move(result.Value()));
                    }
                }
            }
        }
        fts_close(file_system);
        if (!anySuccess)
        {
            OsConfigLogError(log, "No valid systemd config files found in directory '%s'", params.dir->c_str());
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
