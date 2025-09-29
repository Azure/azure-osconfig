// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_DUPLICATE_ENTRIES_EXIST_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_DUPLICATE_ENTRIES_EXIST_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct EnsureNoDuplicateEntriesExistParams
{
    /// The file to be checked for duplicate entries
    std::string filename;

    /// A single character used to separate entries
    std::string delimiter;

    /// Column index to check for duplicates
    int column = 0;

    /// Context for the entries used in the messages
    Optional<std::string> context;
};

Result<Status> AuditEnsureNoDuplicateEntriesExist(const EnsureNoDuplicateEntriesExistParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_NO_DUPLICATE_ENTRIES_EXIST_H
