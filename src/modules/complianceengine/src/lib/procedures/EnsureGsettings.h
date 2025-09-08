// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_GSETTINGS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_GSETTINGS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
enum class GsettingsKeyType
{
    /// label: number
    Number,

    /// label: string
    String,
};

enum class GsettingsOperationType
{
    /// label: eq
    Equal,

    /// label: ne
    NotEqual,

    /// label: lt
    LessThan,

    /// label: gt
    GreaterThan,

    /// label: is-unlocked
    IsUnlocked,
};

struct EnsureGsettingsParams
{
    /// Name of the gsettings schema to get
    std::string schema;

    /// Name of gsettings key to get
    std::string key;

    /// Type of key, possible options string,number
    /// pattern: ^(number|string)$
    GsettingsKeyType keyType = GsettingsKeyType::Number;

    /// Type of operation to perform on variable one of eq, ne, lt, gt,is-unlocked
    /// pattern: ^(eq|ne|lt|gt|is-unlocked)$
    GsettingsOperationType operation = GsettingsOperationType::Equal;

    /// Value of operation to check according to the operation
    std::string value;
};

Result<Status> AuditEnsureGsettings(const EnsureGsettingsParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_GSETTINGS_H
