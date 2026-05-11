// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_MOUNT_POINT_EXISTS_H
#define COMPLIANCEENGINE_PROCEDURES_MOUNT_POINT_EXISTS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct MountPointExistsParams
{
    /// Mount point to check
    std::string mountPoint;
};

Result<Status> AuditMountPointExists(const MountPointExistsParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_MOUNT_POINT_EXISTS_H
