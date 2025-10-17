// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <EnsureShadowContains.h>
#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureShadowContains;
using ComplianceEngine::ComparisonOperation;
using ComplianceEngine::EnsureShadowContainsParams;
using ComplianceEngine::Error;
using ComplianceEngine::Field;
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

    void SetUp() override
    {
        mIndicators.Push("ShadowContains");
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
    EnsureShadowContainsParams params;
    params.field = Field::LastChange;
    params.value = "42";
    params.operation = ComparisonOperation::PatternMatch;
    // Use a non-empty password so the user is not skipped
    const auto path = CreateTestShadowFile("testuser:$6$:0::::::");
    params.test_etcShadowPath = path;
    params.username_operation = ComparisonOperation::Equal; // unused but required by procedure
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Unsupported comparison operation for an integer type");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_2)
{
    EnsureShadowContainsParams params;
    params.field = Field::Username;
    params.value = "test";
    params.operation = ComparisonOperation::PatternMatch;
    const auto path = CreateTestShadowFile("testuser:$6$:0::::::");
    params.test_etcShadowPath = path;
    params.username_operation = ComparisonOperation::Equal;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Username field comparison is not supported");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_3)
{
    EnsureShadowContainsParams params;
    params.field = Field::EncryptionMethod;
    params.value = "asdf";
    params.operation = ComparisonOperation::PatternMatch;
    const auto path = CreateTestShadowFile("testuser:$6$:0::::::");
    params.test_etcShadowPath = path;
    params.username_operation = ComparisonOperation::Equal;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Unsupported comparison operation for encryption method");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureShadowContainsTest, InvalidArguments_4)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser::0::::::");
    params.field = Field::LastChange;
    params.value = "x";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, string("invalid last password change date parameter value"));
}

TEST_F(EnsureShadowContainsTest, SpecificUser_1)
{
    EnsureShadowContainsParams params;
    params.field = Field::Password;
    params.value = "test";
    params.operation = ComparisonOperation::PatternMatch;
    // Create controlled shadow file with password that does NOT contain 'test' so pattern fails
    const auto path = CreateTestShadowFile("testuser:$6$abc$xyz:0::::::");
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_2)
{
    if (0 != getuid())
    {
        GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
    }
    EnsureShadowContainsParams params;
    params.field = Field::Password;
    params.value = "^.*$";
    params.operation = ComparisonOperation::PatternMatch;
    params.username = "root";
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_3)
{
    if (0 != getuid())
    {
        GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
    }
    EnsureShadowContainsParams params;
    params.field = Field::Password;
    params.value = "^.*$";
    params.operation = ComparisonOperation::PatternMatch;
    params.username = "^root$";
    params.username_operation = ComparisonOperation::PatternMatch;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_4)
{
    if (0 != getuid())
    {
        GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
    }
    EnsureShadowContainsParams params;
    params.field = Field::Password;
    params.value = "^test$";
    params.operation = ComparisonOperation::PatternMatch;
    params.username = "^$";
    params.username_operation = ComparisonOperation::PatternMatch;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // No users matched the empty string pattern so we return compliant status
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_5)
{
    if (0 != getuid())
    {
        GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
    }
    EnsureShadowContainsParams params;
    params.field = Field::Password;
    params.value = "^test$";
    params.operation = ComparisonOperation::PatternMatch;
    params.username = "^root$";
    params.username_operation = ComparisonOperation::Equal;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, SpecificUser_6)
{
    EnsureShadowContainsParams params;
    params.field = Field::Password;
    params.value = "^test$";
    params.operation = ComparisonOperation::PatternMatch;
    const auto path = CreateTestShadowFile("testuser:$6$abc$xyz:0::::::");
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_1)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$6$rounds=5000$randomsalt$hashedpassword"));
    params.field = Field::EncryptionMethod;
    params.value = "SHA-512";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
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
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string(""));
    params.field = Field::EncryptionMethod;
    params.value = "SHA-512";
    params.operation = ComparisonOperation::NotEqual;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_3)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("abcd"));
    params.field = Field::EncryptionMethod;
    params.value = "DES";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_4)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("_abcd"));
    params.field = Field::EncryptionMethod;
    params.value = "BSDi";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_5)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("!"));
    params.field = Field::EncryptionMethod;
    params.value = "None";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_6)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("*"));
    params.field = Field::EncryptionMethod;
    params.value = "None";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_7)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$1$"));
    params.field = Field::EncryptionMethod;
    params.value = "MD5";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_8)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$2$"));
    params.field = Field::EncryptionMethod;
    params.value = "Blowfish";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_9)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$2a$"));
    params.field = Field::EncryptionMethod;
    params.value = "Blowfish";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_10)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$2y$"));
    params.field = Field::EncryptionMethod;
    params.value = "Blowfish";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_11)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$md5$"));
    params.field = Field::EncryptionMethod;
    params.value = "MD5";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_12)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$5$"));
    params.field = Field::EncryptionMethod;
    params.value = "SHA-256";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, EncryptionMethod_13)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"));
    params.field = Field::EncryptionMethod;
    params.value = "YesCrypt";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_1)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    params.field = Field::LastChange;
    params.value = "1";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_2)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    params.field = Field::MinAge;
    params.value = "2";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_3)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    params.field = Field::MaxAge;
    params.value = "3";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_4)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    params.field = Field::WarnPeriod;
    params.value = "4";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_5)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    params.field = Field::InactivityPeriod;
    params.value = "5";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, IntegerFields_6)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    params.field = Field::ExpirationDate;
    params.value = "6";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureShadowContainsTest, FeatureFlag)
{
    EnsureShadowContainsParams params;
    const auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    params.field = Field::Reserved;
    params.value = "6";
    params.operation = ComparisonOperation::Equal;
    params.username = "testuser";
    params.username_operation = ComparisonOperation::Equal;
    params.test_etcShadowPath = path;
    auto result = AuditEnsureShadowContains(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, string("reserved field comparison is not supported"));
}
