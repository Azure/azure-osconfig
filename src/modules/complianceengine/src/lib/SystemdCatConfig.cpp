// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <SystemdCatConfig.h>

namespace ComplianceEngine
{
Result<std::string> SystemdCatConfig(const std::string& filename, ContextInterface& context)
{
    auto result = context.ExecuteCommand("systemd-analyze cat-config " + filename);
    if (!result.HasValue())
    {
        return result;
    }

    return result.Value();
}
}; // namespace ComplianceEngine
