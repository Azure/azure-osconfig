// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_FILESYSTEM_OPTION_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_FILESYSTEM_OPTION_H

#include <Evaluator.h>
#include <Separated.h>

namespace ComplianceEngine
{
struct EnsureFilesystemOptionParams
{
    /// Filesystem mount point
    std::string mountpoint;

    /// Comma-separated list of options that must be set
    Optional<Separated<std::string, ','>> optionsSet;

    /// Comma-separated list of options that must not be set
    Optional<Separated<std::string, ','>> optionsNotSet;

    /// Location of the fstab file
    Optional<std::string> test_fstab = std::string("/etc/fstab");

    /// Location of the mtab file
    Optional<std::string> test_mtab = std::string("/etc/mstab");

    /// Location of the mount binary
    Optional<std::string> test_mount = std::string("/sbin/mount");
};

Result<Status> AuditEnsureFilesystemOption(const EnsureFilesystemOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateEnsureFilesystemOption(const EnsureFilesystemOptionParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_FILESYSTEM_OPTION_H
