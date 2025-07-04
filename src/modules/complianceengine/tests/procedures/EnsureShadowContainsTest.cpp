// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

using ComplianceEngine::AuditEnsureShadowContains;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureShadowContainsTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;

    void SetUp() override
    {
        mIndicators.Push("UfwStatus");
    }

    void TearDown() override
    {
    }
};

TEST_F(EnsureShadowContainsTest, InvalidArguments_1)
{
    std::map<std::string, std::string> args;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'field' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_2)
{
    std::map<std::string, std::string> args;
    args["field"] = "x";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid field name: x");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_3)
{
    std::map<std::string, std::string> args;
    args["field"] = "last_change";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'value' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_4)
{
    std::map<std::string, std::string> args;
    args["field"] = "last_change";
    args["value"] = "42";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'operation' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_5)
{
    std::map<std::string, std::string> args;
    args["field"] = "last_change";
    args["value"] = "42";
    args["operation"] = "invalid_op";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid operation: 'invalid_op'");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_6)
{
    std::map<std::string, std::string> args;
    args["field"] = "last_change";
    args["value"] = "42";
    args["operation"] = "match";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Unsupported comparison operation for an integer type");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_7)
{
    std::map<std::string, std::string> args;
    args["field"] = "username";
    args["value"] = "test";
    args["operation"] = "match";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Username field comparison is not supported");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_8)
{
    std::map<std::string, std::string> args;
    args["field"] = "encryption_method";
    args["value"] = "asdf";
    args["operation"] = "match";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Unsupported comparison operation for encryption method");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_1)
{
    std::map<std::string, std::string> args;
    args["field"] = "password";
    args["value"] = "test";
    args["operation"] = "match";
    args["username"] = "root";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_2)
{
    std::map<std::string, std::string> args;
    args["field"] = "password";
    args["value"] = "^.*$";
    args["operation"] = "match";
    args["username"] = "root";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_3)
{
    std::map<std::string, std::string> args;
    args["field"] = "password";
    args["value"] = "^.*$";
    args["operation"] = "match";
    args["username"] = "^root$";
    args["username_operation"] = "match";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_4)
{
    std::map<std::string, std::string> args;
    args["field"] = "password";
    args["value"] = "^test$";
    args["operation"] = "match";
    args["username"] = "^$";
    args["username_operation"] = "match";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // No users matched the empty string pattern so we return compliant status
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_5)
{
    std::map<std::string, std::string> args;
    args["field"] = "password";
    args["value"] = "^test$";
    args["operation"] = "match";
    args["username"] = "^root$";
    args["username_operation"] = "eq";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_6)
{
    std::map<std::string, std::string> args;
    args["field"] = "password";
    args["value"] = "^test$";
    args["operation"] = "match";
    args["username"] = "root";
    args["username_operation"] = "eq";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
