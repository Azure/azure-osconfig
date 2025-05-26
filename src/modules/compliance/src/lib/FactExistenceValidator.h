// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_FACT_EXISTENCE_VALIDATOR_H
#define COMPLIANCE_FACT_EXISTENCE_VALIDATOR_H

#include <MmiResults.h>
#include <Optional.h>
#include <Result.h>
#include <string>

namespace compliance
{
/// Class to encapsulate logic checks for complex Facts.
/// Some complex checks like FileRegexMatch validates multiple aspects of the system.
/// Like, does path exists, does file exists, does file contain expected content, etc.
/// Expressing it as Not { FileRegexMatch path=/etc/ filePattern=sudo.conf matchPattern=NOPASSWD }
/// doesn't clearly show if we want to assert /etc/sudo.conf not to exist or assert it should not contain NOPASSWD.
///
/// FactExistenceValidator supports following Behaviors:
/// AllExist - Fact is Compliant if and only if all Criteria are Met (called CriteriaMet) and exists (think like logical and)
/// AnyExist - Fact is Compliant if and only if at least one of Criteria is met, and exists (think like logical or)
/// NoneExist - Fact is Compliant if and only if all Criteria are Unmet
/// OnlyOneExists - Fact is Compliant if and only if only one of Criteria is met
/// AtLeastOneExists - Fact is Compliant if and only if one or more of Criteria is met
class FactExistenceValidator
{
public:
    enum class Behavior
    {
        AllExist,
        AnyExist,
        NoneExist,
        OnlyOneExists,
        AtLeastOneExists,
    };

    explicit FactExistenceValidator(Behavior behavior);
    FactExistenceValidator(const FactExistenceValidator&) = default;
    FactExistenceValidator(FactExistenceValidator&&) = default;
    FactExistenceValidator& operator=(const FactExistenceValidator&) = default;
    FactExistenceValidator& operator=(FactExistenceValidator&&) = default;
    ~FactExistenceValidator() = default;

    /// Used by Fact to 'save state' of check executed successfully (regardless of behavior)
    /// e.g when { FileRegexMatch path=/etc/ filePattern=sudo.conf matchPattern=NOPASSWD }
    ///  CriteriaMet is when path, filePattern and matchPattern exists
    void CriteriaMet();
    /// Used by Fact to 'save state' of check executed unsuccessfully (regardless of behavior)
    /// e.g when { FileRegexMatch path=/etc/ filePattern=sudo.conf matchPattern=NOPASSWD }
    ///  CriteriaUnmet eg. when file is not found
    void CriteriaUnmet();
    /// Used by Fact notify FactExistenceValidator that evaluation stooped and Result is present
    void Finish();
    /// returns True if FactExistenceValidator can evaluate Result
    bool Done() const;
    /// returns Result of validation (Compliant or NonCompliant)
    Status Result() const;

    static compliance::Result<Behavior> MapBehavior(const std::string& value);

private:
    Behavior mBehavior;
    Optional<Status> mState;
    bool mHasAtLeastOneFact = false;
};
} // namespace compliance

#endif // COMPLIANCE_FACT_EXISTENCE_VALIDATOR_H
