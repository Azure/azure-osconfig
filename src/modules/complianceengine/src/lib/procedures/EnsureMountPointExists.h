// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_MOUNT_POINT_EXISTS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_MOUNT_POINT_EXISTS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct EnsureMountPointExistsParams
{
    /// Mount point to check
    std::string mountPoint;
};

Result<Status> AuditEnsureMountPointExists(const EnsureMountPointExistsParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_MOUNT_POINT_EXISTS_H
