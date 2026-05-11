// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_FILESYSTEM_MOUNT_OPTION_H
#define COMPLIANCEENGINE_PROCEDURES_FILESYSTEM_MOUNT_OPTION_H

#include <Evaluator.h>
#include <Separated.h>

namespace ComplianceEngine
{
struct FilesystemMountOptionParams
{
    /// Filesystem mount point
    std::string mountpoint;

    /// Comma-separated list of options that must be set
    Optional<Separated<std::string, ','>> optionsSet;

    /// Comma-separated list of options that must not be set
    Optional<Separated<std::string, ','>> optionsNotSet;
};

Result<Status> AuditFilesystemMountOption(const FilesystemMountOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateFilesystemMountOption(const FilesystemMountOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_FILESYSTEM_MOUNT_OPTION_H
