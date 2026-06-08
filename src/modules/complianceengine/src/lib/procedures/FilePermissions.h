// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_FILE_PERMISSIONS_H
#define COMPLIANCEENGINE_PROCEDURES_FILE_PERMISSIONS_H

#include <Behavior.h>
#include <Evaluator.h>
#include <Pattern.h>
#include <Separated.h>

namespace ComplianceEngine
{

struct FilePermissionsParams
{
    /// Path to the file
    std::string path;

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

    /// Behavior when checking file existence
    Optional<Behavior> behavior = Behavior::AtLeastOneExists;
};

Result<Status> AuditFilePermissions(const FilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateFilePermissions(const FilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context);

struct FilePermissionsCollectionParams
{
    /// Directory path
    std::string directory;

    /// Whether to recurse
    Optional<bool> recurse = true;

    /// File pattern
    std::string filePattern;

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

    /// Behavior when checking file existence
    Optional<Behavior> behavior = Behavior::AtLeastOneExists;
};

Result<Status> AuditFilePermissionsCollection(const FilePermissionsCollectionParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateFilePermissionsCollection(const FilePermissionsCollectionParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_FILE_PERMISSIONS_H
