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
class FactExistenceValidator
{
public:
    enum class Behavior
    {
        AllExist,
        AnyExist,
        AtLeastOneExists,
        NoneExist,
        OnlyOneExists,
    };

    explicit FactExistenceValidator(Behavior behavior);
    FactExistenceValidator(const FactExistenceValidator&) = default;
    FactExistenceValidator(FactExistenceValidator&&) = default;
    FactExistenceValidator& operator=(const FactExistenceValidator&) = default;
    FactExistenceValidator& operator=(FactExistenceValidator&&) = default;
    ~FactExistenceValidator() = default;

    void Finish();
    void CriteriaMet();
    void CriteriaUnmet();
    bool Done() const;
    Status Result() const;
    static compliance::Result<Behavior> MapBehavior(const std::string& value);

private:
    Behavior mBehavior;
    Optional<Status> mState;
    bool mHasAtLeastOneFact = false;
};
} // namespace compliance

#endif // COMPLIANCE_FACT_EXISTENCE_VALIDATOR_H
