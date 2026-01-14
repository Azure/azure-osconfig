// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_SYSTEMD_CAT_CONFIG_H
#define COMPLIANCEENGINE_SYSTEMD_CAT_CONFIG_H

#include <ContextInterface.h>
#include <Result.h>

namespace ComplianceEngine
{
Result<std::string> SystemdCatConfig(const std::string& filename, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_SYSTEMD_CAT_CONFIG_H
