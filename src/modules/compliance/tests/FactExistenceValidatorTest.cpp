// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "FactExistenceValidator.h"

#include <gtest/gtest.h>

using compliance::Error;
using compliance::FactExistenceValidator;
using compliance::Result;

class FactExistenceValidatorTest : public ::testing::Test
{
};

TEST_F(FactExistenceValidatorTest, MapBehavior)
{
    auto result = compliance::FactExistenceValidator::MapBehavior("all_exist");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), compliance::FactExistenceValidator::Behavior::AllExist);

    result = compliance::FactExistenceValidator::MapBehavior("any_exist");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), compliance::FactExistenceValidator::Behavior::AnyExist);

    result = compliance::FactExistenceValidator::MapBehavior("at_least_one_exists");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), compliance::FactExistenceValidator::Behavior::AtLeastOneExists);

    result = compliance::FactExistenceValidator::MapBehavior("none_exist");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), compliance::FactExistenceValidator::Behavior::NoneExist);

    result = compliance::FactExistenceValidator::MapBehavior("only_one_exists");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), compliance::FactExistenceValidator::Behavior::OnlyOneExists);

    result = compliance::FactExistenceValidator::MapBehavior("invalid_value");
    ASSERT_FALSE(result.HasValue());
}

TEST_F(FactExistenceValidatorTest, AllExist_1)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AllExist);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);

    // Already done, Finish should not affact the state
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AllExist_2)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AllExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaUnmet();
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AllExist_3)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AllExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, AnyExist_1)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AnyExist);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AnyExist_2)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AnyExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AnyExist_3)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AnyExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AtLeastOneExists_1)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AtLeastOneExists);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, AtLeastOneExists_2)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AtLeastOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::NonCompliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), compliance::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, AtLeastOneExists_3)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::AtLeastOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, NoneExist_1)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::NoneExist);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, NoneExist_2)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::NoneExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, NoneExist_3)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::NoneExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_TRUE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_TRUE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, OnlyOneExists_1)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::OnlyOneExists);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, OnlyOneExists_2)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::OnlyOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), compliance::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, OnlyOneExists_3)
{
    FactExistenceValidator validator(compliance::FactExistenceValidator::Behavior::OnlyOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), compliance::Status::NonCompliant);
}
