// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_DCONF_VALUE_H
#define COMPLIANCEENGINE_PROCEDURES_DCONF_VALUE_H

#include <Evaluator.h>

namespace ComplianceEngine
{
enum class DConfOperation
{
    /// label: eq
    Eq,

    /// label: ne
    Ne,
};

struct DconfValueParams
{
    /// dconf key name to be checked
    std::string key;

    /// Value to be verified using the operation
    std::string value;

    /// Type of operation, one of eq, ne
    /// pattern: ^(eq|ne)$
    DConfOperation operation = DConfOperation::Eq;
};

Result<Status> AuditDconfValue(const DconfValueParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_DCONF_VALUE_H
