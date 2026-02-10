// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <SystemdConfig.h>
#include <Telemetry.h>
#include <algorithm>
#include <cstdlib>
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
// Maps (block, parameter) -> (value, sourceFile)
typedef std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string>> SystemdConfigMap_t;

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
    std::string currentBlock;
    while (std::getline(stream, line))
    {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
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
        if (line.front() == '[' && line.back() == ']')
        {
            currentBlock = line;
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
        config[std::make_pair(currentBlock, key)] = std::make_pair(value, currentConfig);
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
            OSConfigTelemetryStatusTrace("fts_open", EINVAL);
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
            OSConfigTelemetryStatusTrace("fts_close", EINVAL);
            return Error("No valid systemd config files found in directory '" + params.dir.Value() + "'");
        }
    }

    if (params.valueRegex.HasValue() && params.value.HasValue())
    {
        OsConfigLogError(log, "Error: SystemdParameter: both 'value' and 'valueRegex' are provided, only one is allowed");
        OSConfigTelemetryStatusTrace("value and valueRegex", EINVAL);
        return Error("Both 'value' and 'valueRegex' are provided, only one is allowed");
    }
    if (!params.valueRegex.HasValue() && !params.value.HasValue())
    {
        OsConfigLogError(log, "Error: SystemdParameter: 'value' (or 'valueRegex') must be provided");
        OSConfigTelemetryStatusTrace("value required", EINVAL);
        return Error("'value' (or 'valueRegex') must be provided");
    }
    if (params.op.HasValue() && !params.value.HasValue())
    {
        OsConfigLogError(log, "Error: SystemdParameter: 'op' requires 'value' (not 'valueRegex')");
        OSConfigTelemetryStatusTrace("op requires value", EINVAL);
        return Error("'op' requires 'value' (not 'valueRegex')");
    }

    SystemdConfigMap_t::const_iterator paramIt = config.end();
    if (params.block.HasValue())
    {
        paramIt = config.find(std::make_pair(params.block.Value(), params.parameter));
    }
    else
    {
        for (auto it = config.begin(); it != config.end(); ++it)
        {
            if (it->first.second == params.parameter)
            {
                paramIt = it;
                break;
            }
        }
    }

    if (paramIt == config.end())
    {
        if (params.block.HasValue())
        {
            OsConfigLogInfo(log, "Parameter '%s' not found in block '%s'", params.parameter.c_str(), params.block->c_str());
            return indicators.NonCompliant("Parameter '" + params.parameter + "' not found in block '" + params.block.Value() + "'");
        }
        else
        {
            OsConfigLogInfo(log, "Parameter '%s' not found", params.parameter.c_str());
            return indicators.NonCompliant("Parameter '" + params.parameter + "' not found");
        }
    }

    const std::string& actualValue = paramIt->second.first;
    const std::string& sourceFile = paramIt->second.second;

    OsConfigLogDebug(log, "Parameter '%s' found in file '%s' with value '%s'", params.parameter.c_str(), sourceFile.c_str(), actualValue.c_str());

    if (params.valueRegex.HasValue())
    {
        if (!regex_match(actualValue, params.valueRegex.Value()))
        {
            OsConfigLogInfo(log, "Parameter '%s' in file '%s' does not match regex", params.parameter.c_str(), sourceFile.c_str());
            return indicators.NonCompliant("Parameter '" + params.parameter + "' value '" + actualValue + "' in file '" + sourceFile +
                                           "' does not match regex");
        }
        else
        {
            return indicators.Compliant("Parameter '" + params.parameter + "' found in file '" + sourceFile + "' with value '" + actualValue + "'");
        }
    }
    else if (params.value.HasValue() && !params.op.HasValue())
    {
        // value without op: treat value as regex pattern
        regex valueAsRegex(params.value.Value());
        if (!regex_match(actualValue, valueAsRegex))
        {
            OsConfigLogInfo(log, "Parameter '%s' in file '%s' does not match regex", params.parameter.c_str(), sourceFile.c_str());
            return indicators.NonCompliant("Parameter '" + params.parameter + "' value '" + actualValue + "' in file '" + sourceFile +
                                           "' does not match regex");
        }
        else
        {
            return indicators.Compliant("Parameter '" + params.parameter + "' found in file '" + sourceFile + "' with value '" + actualValue + "'");
        }
    }
    else
    {
        // Operator + value comparison
        bool comparisonResult = false;
        const std::string& expectedValue = params.value.Value();
        SystemdParameterOperator op = params.op.Value();

        if (op == SystemdParameterOperator::Equal)
        {
            comparisonResult = (actualValue == expectedValue);
        }
        else
        {
            // Numerical comparison for lt, le, gt, ge
            char* endActual = nullptr;
            char* endExpected = nullptr;
            long actualNum = strtol(actualValue.c_str(), &endActual, 10);
            long expectedNum = strtol(expectedValue.c_str(), &endExpected, 10);

            if (endActual == actualValue.c_str() || endExpected == expectedValue.c_str())
            {
                OsConfigLogError(log, "Failed to convert values to numbers for comparison: actual='%s', expected='%s'", actualValue.c_str(),
                    expectedValue.c_str());
                OSConfigTelemetryStatusTrace("strtol", EINVAL);
                return Error("Failed to convert values to numbers for comparison: actual='" + actualValue + "', expected='" + expectedValue + "'");
            }

            switch (op)
            {
                case SystemdParameterOperator::LessThan:
                    comparisonResult = (actualNum < expectedNum);
                    break;
                case SystemdParameterOperator::LessOrEqual:
                    comparisonResult = (actualNum <= expectedNum);
                    break;
                case SystemdParameterOperator::GreaterThan:
                    comparisonResult = (actualNum > expectedNum);
                    break;
                case SystemdParameterOperator::GreaterOrEqual:
                    comparisonResult = (actualNum >= expectedNum);
                    break;
                default:
                    break;
            }
        }

        if (comparisonResult)
        {
            return indicators.Compliant("Parameter '" + params.parameter + "' found in file '" + sourceFile + "' with value '" + actualValue + "'");
        }
        else
        {
            return indicators.NonCompliant("Parameter '" + params.parameter + "' value '" + actualValue + "' in file '" + sourceFile +
                                           "' does not satisfy the comparison");
        }
    }
}

} // namespace ComplianceEngine
