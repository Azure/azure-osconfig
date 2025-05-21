// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <FactExistenceValidator.h>
#include <cassert>
#include <map>

namespace compliance
{
FactExistenceValidator::FactExistenceValidator(Behavior behavior)
    : mBehavior(behavior)
{
}

void FactExistenceValidator::Finish()
{
    if (Done())
    {
        return;
    }

    switch (mBehavior)
    {
        case Behavior::AllExist:
            mState = Status::Compliant;
            break;
        case Behavior::NoneExist:
            assert(!mHasAtLeastOneFact);
            mState = Status::Compliant;
            break;

        case Behavior::AtLeastOneExists:
            assert(!mHasAtLeastOneFact);
            mState = Status::NonCompliant;
            break;

        case Behavior::AnyExist:
            assert(!mHasAtLeastOneFact);
            mState = Status::Compliant;
            break;

        case Behavior::OnlyOneExists:
            mState = mHasAtLeastOneFact ? Status::Compliant : Status::NonCompliant;
            break;
    }
    assert(Done());
}

void FactExistenceValidator::CriteriaMet()
{
    if (Done())
    {
        return;
    }

    switch (mBehavior)
    {
        case Behavior::AllExist:
            break;

        case Behavior::AnyExist:
            mState = Status::Compliant;
            break;

        case Behavior::AtLeastOneExists:
            mState = Status::Compliant;
            break;

        case Behavior::NoneExist:
            mState = Status::NonCompliant;
            break;

        case Behavior::OnlyOneExists:
            if (mHasAtLeastOneFact)
            {
                mState = Status::NonCompliant;
            }
            break;
    }
    mHasAtLeastOneFact = true;
}

void FactExistenceValidator::CriteriaUnmet()
{
    if (Done())
    {
        return;
    }

    switch (mBehavior)
    {
        case Behavior::AllExist:
            mState = Status::NonCompliant;
            break;
        case Behavior::NoneExist:
            mState = Status::Compliant;
            break;
        default:
            break;
    }
}

bool FactExistenceValidator::Done() const
{
    return mState.HasValue();
}

Status FactExistenceValidator::Result() const
{
    assert(mState.HasValue());
    return mState.Value();
}

compliance::Result<FactExistenceValidator::Behavior> FactExistenceValidator::MapBehavior(const std::string& value)
{
    static const std::map<std::string, Behavior> sMap = {
        {"all_exist", Behavior::AllExist},
        {"any_exist", Behavior::AnyExist},
        {"at_least_one_exists", Behavior::AtLeastOneExists},
        {"none_exist", Behavior::NoneExist},
        {"only_one_exists", Behavior::OnlyOneExists},
    };

    auto it = sMap.find(value);
    if (it == sMap.end())
    {
        return Error("unsupported value: " + value, EINVAL);
    }

    return it->second;
}
} // namespace compliance
