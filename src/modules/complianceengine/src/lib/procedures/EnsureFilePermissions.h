// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_FILE_PERMISSIONS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_FILE_PERMISSIONS_H

#include <Evaluator.h>
#include <Pattern.h>
#include <Separated.h>

namespace ComplianceEngine
{
struct EnsureFilePermissionsParams
{
    /// Path to the file
    std::string filename;

    /// Required owner of the file, single or | separated, first one is used for remediation
    Optional<Separated<Pattern, '|'>> owner;

    /// Required group of the file, single or | separated, first one is used for remediation
    Optional<Separated<Pattern, '|'>> group;

    /// Required octal permissions of the file
    /// pattern: ^[0-7]{3,4}$
    Optional<mode_t> permissions;

    /// Required octal permissions of the file - mask
    /// pattern: ^[0-7]{3,4}$
    Optional<mode_t> mask;
};

Result<Status> AuditEnsureFilePermissions(const EnsureFilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateEnsureFilePermissions(const EnsureFilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context);

struct EnsureFilePermissionsCollectionParams
{
    /// Directory path
    std::string directory;

    /// File pattern
    std::string ext;

    /// Required owner of the file, single or | separated, first one is used for remediation
    Optional<Separated<Pattern, '|'>> owner;

    /// Required group of the file, single or | separated, first one is used for remediation
    Optional<Separated<Pattern, '|'>> group;

    /// Required octal permissions of the file
    /// pattern: ^[0-7]{3,4}$
    Optional<mode_t> permissions;

    /// Required octal permissions of the file - mask
    /// pattern: ^[0-7]{3,4}$
    Optional<mode_t> mask;
};

Result<Status> AuditEnsureFilePermissionsCollection(const EnsureFilePermissionsCollectionParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateEnsureFilePermissionsCollection(const EnsureFilePermissionsCollectionParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_FILE_PERMISSIONS_H
