// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "FactExistenceValidator.h"

#include <gtest/gtest.h>

using ComplianceEngine::Error;
using ComplianceEngine::FactExistenceValidator;
using ComplianceEngine::Result;

class FactExistenceValidatorTest : public ::testing::Test
{
};

TEST_F(FactExistenceValidatorTest, MapBehavior)
{
    auto result = ComplianceEngine::FactExistenceValidator::MapBehavior("all_exist");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), ComplianceEngine::FactExistenceValidator::Behavior::AllExist);

    result = ComplianceEngine::FactExistenceValidator::MapBehavior("any_exist");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), ComplianceEngine::FactExistenceValidator::Behavior::AnyExist);

    result = ComplianceEngine::FactExistenceValidator::MapBehavior("at_least_one_exists");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), ComplianceEngine::FactExistenceValidator::Behavior::AtLeastOneExists);

    result = ComplianceEngine::FactExistenceValidator::MapBehavior("none_exist");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), ComplianceEngine::FactExistenceValidator::Behavior::NoneExist);

    result = ComplianceEngine::FactExistenceValidator::MapBehavior("only_one_exists");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), ComplianceEngine::FactExistenceValidator::Behavior::OnlyOneExists);

    result = ComplianceEngine::FactExistenceValidator::MapBehavior("invalid_value");
    ASSERT_FALSE(result.HasValue());
}

TEST_F(FactExistenceValidatorTest, AllExist_1)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AllExist);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);

    // Already done, Finish should not affact the state
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AllExist_2)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AllExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaUnmet();
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AllExist_3)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AllExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, AnyExist_1)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AnyExist);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AnyExist_2)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AnyExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AnyExist_3)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AnyExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, AtLeastOneExists_1)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AtLeastOneExists);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, AtLeastOneExists_2)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AtLeastOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::NonCompliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, AtLeastOneExists_3)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::AtLeastOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, NoneExist_1)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::NoneExist);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, NoneExist_2)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::NoneExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, NoneExist_3)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::NoneExist);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_TRUE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_TRUE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, OnlyOneExists_1)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::OnlyOneExists);
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::NonCompliant);
}

TEST_F(FactExistenceValidatorTest, OnlyOneExists_2)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::OnlyOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.Finish();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
    // Already done, should not change state anymore
    validator.CriteriaMet();
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::Compliant);
}

TEST_F(FactExistenceValidatorTest, OnlyOneExists_3)
{
    FactExistenceValidator validator(ComplianceEngine::FactExistenceValidator::Behavior::OnlyOneExists);
    ASSERT_FALSE(validator.Done());
    validator.CriteriaUnmet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_FALSE(validator.Done());
    validator.CriteriaMet();
    ASSERT_TRUE(validator.Done());
    ASSERT_EQ(validator.Result(), ComplianceEngine::Status::NonCompliant);
}
