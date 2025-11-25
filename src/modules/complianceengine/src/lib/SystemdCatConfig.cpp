// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <SystemdCatConfig.h>
#include <Telemetry.h>

namespace ComplianceEngine
{
namespace
{
Result<std::string> DetermineCommandPath(const std::string& command, const ContextInterface& context)
{
    auto result = context.ExecuteCommand("readlink -e /bin/" + command);
    if (result.HasValue())
    {
        OsConfigLogInfo(context.GetLogHandle(), "'%s' path is: %s", command.c_str(), result->c_str());
        return std::move(result.Value());
    }

    result = context.ExecuteCommand("readlink -e /usr/bin/" + command);
    if (result.HasValue())
    {
        OsConfigLogInfo(context.GetLogHandle(), "'%s' path is: %s", command.c_str(), result->c_str());
        return std::move(result.Value());
    }

    OsConfigLogError(context.GetLogHandle(), "Failed to determine systemd-analyze command path");
    return result.Error();
}
} // anonymous namespace

Result<std::string> SystemdCatConfig(const std::string& filename, const ContextInterface& context)
{
    static const auto commandPath = DetermineCommandPath("systemd-analyze", context);
    if (!commandPath.HasValue())
    {
        return commandPath.Error();
    }

    auto result = context.ExecuteCommand(commandPath.Value() + " cat-config " + filename);
    if (!result.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to execute systemd-analyze command: %s", result.Error().message.c_str());
        OSConfigTelemetryStatusTrace("ExecuteCommand", result.Error().code);
        return result;
    }

    return result.Value();
}
}; // namespace ComplianceEngine
