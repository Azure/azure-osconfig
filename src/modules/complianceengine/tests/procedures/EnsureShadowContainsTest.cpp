// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureShadowContains;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsureShadowContainsTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;
    string mTempDir;

    // test
    // comment
    // test

    void SetUp() override
    {
        mIndicators.Push("UfwStatus");
        char tempDirTemplate[] = "/tmp/EnsureShadowContainsTestXXXXXX";
        char* tempDir = mkdtemp(tempDirTemplate);
        ASSERT_NE(tempDir, nullptr);
        mTempDir = tempDir;
    }

    void TearDown() override
    {
        if (!mTempDir.empty())
        {
            if (0 != remove(mTempDir.c_str()))
            {
                OsConfigLogError(mContext.GetLogHandle(), "Failed to remove temporary directory %s: %s", mTempDir.c_str(), strerror(errno));
            }
            mTempDir.clear();
        }
    }

    string CreateTestShadowFile(string username, Optional<string> password, Optional<int> lastChange = Optional<int>(),
        Optional<int> minAge = Optional<int>(), Optional<int> maxAge = Optional<int>(), Optional<int> warnPeriod = Optional<int>(),
        Optional<int> inactivityPeriod = Optional<int>(), Optional<int> expirationDate = Optional<int>())
    {
        auto content = std::move(username);
        content += ":" + (password.HasValue() ? password.Value() : "");
        content += ":" + (lastChange.HasValue() ? std::to_string(lastChange.Value()) : "");
        content += ":" + (minAge.HasValue() ? std::to_string(minAge.Value()) : "");
        content += ":" + (maxAge.HasValue() ? std::to_string(maxAge.Value()) : "");
        content += ":" + (warnPeriod.HasValue() ? std::to_string(warnPeriod.Value()) : "");
        content += ":" + (inactivityPeriod.HasValue() ? std::to_string(inactivityPeriod.Value()) : "");
        content += ":" + (expirationDate.HasValue() ? std::to_string(expirationDate.Value()) : "");
        content += ":";

        return CreateTestShadowFile(std::move(content));
    }

    string CreateTestShadowFile(string content)
    {
        string shadowFilePath = mTempDir + "/shadow";
        std::ofstream shadowFile(shadowFilePath);
        if (!shadowFile.is_open())
        {
            OsConfigLogError(mContext.GetLogHandle(), "Failed to create test shadow file %s: %s", shadowFilePath.c_str(), strerror(errno));
            return string();
        }
        shadowFile << std::move(content);
        shadowFile.close();
        return shadowFilePath;
    }

    void RemoveTestShadowFile(const string& shadowFilePath)
    {
        if (shadowFilePath.empty())
        {
            return;
        }

        if (0 != remove(shadowFilePath.c_str()))
        {
            OsConfigLogError(mContext.GetLogHandle(), "Failed to remove test shadow file %s: %s", shadowFilePath.c_str(), strerror(errno));
        }
    }
};

TEST_F(EnsureShadowContainsTest, InvalidArguments_1)
{
    map<string, string> args;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'field' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_2)
{
    map<string, string> args;
    args["field"] = "x";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid field name: x");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_3)
{
    map<string, string> args;
    args["field"] = "last_change";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'value' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_4)
{
    map<string, string> args;
    args["field"] = "last_change";
    args["value"] = "42";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'operation' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_5)
{
    map<string, string> args;
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
    map<string, string> args;
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
    map<string, string> args;
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
    map<string, string> args;
    args["field"] = "encryption_method";
    args["value"] = "asdf";
    args["operation"] = "match";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Unsupported comparison operation for encryption method");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_9)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser::0::::::");
    args["field"] = "last_change";
    args["value"] = "x";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, string("invalid last password change date parameter value"));
}

TEST_F(EnsureShadowContainsTest, SpecificUser_1)
{
    map<string, string> args;
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
    map<string, string> args;
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
    map<string, string> args;
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
    map<string, string> args;
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
    map<string, string> args;
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
    map<string, string> args;
    args["field"] = "password";
    args["value"] = "^test$";
    args["operation"] = "match";
    args["username"] = "root";
    args["username_operation"] = "eq";
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_1)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$6$rounds=5000$randomsalt$hashedpassword"));
    args["field"] = "encryption_method";
    args["value"] = "SHA-512";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "AuditEnsureShadowContains failed: %s", result.Error().message.c_str());
    }
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_2)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string(""));
    args["field"] = "encryption_method";
    args["value"] = "SHA-512";
    args["operation"] = "ne";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_3)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("abcd"));
    args["field"] = "encryption_method";
    args["value"] = "DES";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_4)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("_abcd"));
    args["field"] = "encryption_method";
    args["value"] = "BSDi";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_5)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("!"));
    args["field"] = "encryption_method";
    args["value"] = "None";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_6)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("*"));
    args["field"] = "encryption_method";
    args["value"] = "None";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_7)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$1$"));
    args["field"] = "encryption_method";
    args["value"] = "MD5";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_8)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$2$"));
    args["field"] = "encryption_method";
    args["value"] = "Blowfish";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_9)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$2a$"));
    args["field"] = "encryption_method";
    args["value"] = "Blowfish";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_10)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$2y$"));
    args["field"] = "encryption_method";
    args["value"] = "Blowfish";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_11)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$md5$"));
    args["field"] = "encryption_method";
    args["value"] = "MD5";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_12)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$5$"));
    args["field"] = "encryption_method";
    args["value"] = "SHA-256";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_13)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"));
    args["field"] = "encryption_method";
    args["value"] = "YesCrypt";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_1)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["field"] = "last_change";
    args["value"] = "1";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_2)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["field"] = "min_age";
    args["value"] = "2";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_3)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["field"] = "max_age";
    args["value"] = "3";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_4)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["field"] = "warn_period";
    args["value"] = "4";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_5)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["field"] = "inactivity_period";
    args["value"] = "5";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_6)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["field"] = "expiration_date";
    args["value"] = "6";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, FeatureFlag)
{
    map<string, string> args;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["field"] = "flag";
    args["value"] = "6";
    args["operation"] = "eq";
    args["username"] = "testuser";
    args["username_operation"] = "eq";
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsureShadowContains(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, string("reserved field comparison is not supported"));
}
