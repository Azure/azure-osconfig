// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_SSH_KEY_PERMISSIONS_H
#define COMPLIANCEENGINE_PROCEDURES_SSH_KEY_PERMISSIONS_H

#include <Evaluator.h>
namespace ComplianceEngine
{
enum class SshKeyType
{
    /// label: public
    Public,

    /// label: private
    Private,
};

struct SshKeyPermissionsParams
{
    /// Key type - public or private
    /// pattern: ^(public|private)$
    SshKeyType type;
};

Result<Status> AuditSshKeyPermissions(const SshKeyPermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateSshKeyPermissions(const SshKeyPermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_SSH_KEY_PERMISSIONS_H
