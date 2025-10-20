// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <SystemdConfig.h>
#include <sstream>

namespace ComplianceEngine
{
Result<SystemdConfig> GetSystemdConfig(const std::string& filename, ContextInterface& context)
{
    auto result = context.ExecuteCommand("/usr/bin/systemd-analyze cat-config " + context.GetSpecialFilePath(filename));
    if (!result.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to execute systemd-analyze command: %s", result.Error().message.c_str());
        return result.Error();
    }

    SystemdConfig config;
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
        const size_t eqSign = line.find('=');
        if (eqSign == std::string::npos)
        {
            OsConfigLogError(context.GetLogHandle(), "Invalid line in systemd config: %s", line.c_str());
            continue;
        }
        std::string key = line.substr(0, eqSign);
        std::string value = line.substr(eqSign + 1);
        config[key] = std::make_pair(std::move(value), currentConfig);
    }
    return config;
}
} // namespace ComplianceEngine
