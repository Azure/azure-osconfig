// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_APPARMOR_PROFILES_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_APPARMOR_PROFILES_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct AuditEnsureApparmorProfilesParams
{
    /// Set for enforce (L2) mode, complain (L1) mode by default
    Optional<bool> enforce = false;
};

Result<Status> AuditEnsureApparmorProfiles(const AuditEnsureApparmorProfilesParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_APPARMOR_PROFILES_H
