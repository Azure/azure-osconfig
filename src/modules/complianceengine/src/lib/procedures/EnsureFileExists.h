// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_FILE_EXISTS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_FILE_EXISTS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
// Parameters used by the EnsureFileExists procedure.
struct AuditEnsureFileExistsParams
{
    /// A filename containing to check for existence
    std::string filename;
};

Result<Status> AuditEnsureFileExists(const AuditEnsureFileExistsParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_FILE_EXISTS_H
