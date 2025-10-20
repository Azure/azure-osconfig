// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_SYSTEMD_CONFIG_H
#define COMPLIANCEENGINE_SYSTEMD_CONFIG_H

#include <ContextInterface.h>
#include <Result.h>
#include <map>
#include <string>

namespace ComplianceEngine
{
struct SystemdConfig : std::map<std::string, std::pair<std::string, std::string>>
{
    void Merge(const SystemdConfig& other) &
    {
        for (const auto& pair : other)
        {
            const auto& key = pair.first;
            const auto& value = pair.second;
            (*this)[key] = value;
        }
    }

    void Merge(SystemdConfig&& other) &
    {
        for (auto&& pair : std::move(other))
        {
            const auto& key = pair.first;
            auto&& value = pair.second;
            (*this)[key] = std::move(value);
        }
    }
};

Result<SystemdConfig> GetSystemdConfig(const std::string& filename, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_SYSTEMD_CONFIG_H
