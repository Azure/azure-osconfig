// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHKEY_PERMS
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHKEY_PERMS

#include <Evaluator.h>
namespace ComplianceEngine
{
enum class SshKeyType
{
    Public,
    Private
};

struct EnsureSshKeyPermsParams
{
    /// Key type - public or private
    /// pattern: ^(public|private)$
    SshKeyType type;
};

Result<Status> AuditEnsureSshKeyPerms(const EnsureSshKeyPermsParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateEnsureSshKeyPerms(const EnsureSshKeyPermsParams& params, IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SSHKEY_PERMS
