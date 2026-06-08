// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <StringTools.h>
#include <SystemdCatConfig.h>

namespace ComplianceEngine
{
Result<std::string> SystemdCatConfig(const std::string& filename, ContextInterface& context)
{
    // Security: Escape filename to prevent command injection
    auto escapedFilename = EscapeForShell(filename);
    auto result = context.ExecuteCommand("systemd-analyze cat-config \"" + escapedFilename + "\"");
    if (!result.HasValue())
    {
        return result;
    }

    return result.Value();
}
}; // namespace ComplianceEngine
