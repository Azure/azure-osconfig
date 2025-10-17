// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SHADOW_CONTAINS_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SHADOW_CONTAINS_H

#include <Evaluator.h>

namespace ComplianceEngine
{
enum class ComparisonOperation
{
    /// label: eq
    Equal,

    /// label: ne
    NotEqual,

    /// label: lt
    LessThan,

    /// label: le
    LessOrEqual,

    /// label: gt
    GreaterThan,

    /// label: ge
    GreaterOrEqual,

    /// label: match
    PatternMatch,
};

enum class Field
{
    /// label: username
    Username,

    /// label: password
    Password,

    /// label: last_change
    LastChange,

    /// label: min_age
    MinAge,

    /// label: max_age
    MaxAge,

    /// label: warn_period
    WarnPeriod,

    /// label: inactivity_period
    InactivityPeriod,

    /// label: expiration_date
    ExpirationDate,

    /// label: reserved
    Reserved,

    /// label: encryption_method
    EncryptionMethod,
};

struct EnsureShadowContainsParams
{
    /// A pattern or value to match usernames against
    Optional<std::string> username;

    /// A comparison operation for the username parameter
    /// pattern: ^(eq|ne|lt|le|gt|ge|match)$
    Optional<ComparisonOperation> username_operation = ComparisonOperation::Equal;

    /// The /etc/shadow entry field to match against
    /// pattern: ^(password|last_change|min_age|max_age|warn_period|inactivity_period|expiration_date|encryption_method)$
    Field field = Field::LastChange;

    /// A pattern or value to match against the specified field
    std::string value;

    /// A comparison operation for the value parameter
    /// pattern: ^(eq|ne|lt|le|gt|ge|match)$
    ComparisonOperation operation = ComparisonOperation::Equal;

    /// Path to the /etc/shadow file to test against
    Optional<std::string> test_etcShadowPath = std::string("/etc/shadow");
};

Result<Status> AuditEnsureShadowContains(const EnsureShadowContainsParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SHADOW_CONTAINS_H
