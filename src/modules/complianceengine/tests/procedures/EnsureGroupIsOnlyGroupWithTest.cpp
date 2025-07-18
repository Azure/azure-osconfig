// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureGroupIsOnlyGroupWith;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsureGroupIsOnlyGroupWithTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;
    string mTempDir;

    void SetUp() override
    {
        mIndicators.Push("UfwStatus");
        char tempDirTemplate[] = "/tmp/EnsureGroupIsOnlyGroupWithTestXXXXXX";
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

    string CreateTestGroupFile(string groupName, Optional<string> password, Optional<int> gid = Optional<int>(), Optional<string> users = Optional<string>())
    {
        auto content = std::move(groupName);
        content += ":" + (password.HasValue() ? password.Value() : "");
        content += ":" + (gid.HasValue() ? std::to_string(gid.Value()) : "");
        content += ":" + (users.HasValue() ? users.Value() : "");

        return CreateTestGroupFile(std::move(content));
    }

    string CreateTestGroupFile(string content)
    {
        string shadowFilePath = mTempDir + "/group";
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

TEST_F(EnsureGroupIsOnlyGroupWithTest, InvalidArguments_1)
{
    map<string, string> args;
    auto result = AuditEnsureGroupIsOnlyGroupWith(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'group' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(EnsureGroupIsOnlyGroupWithTest, EmptyFile)
{
    auto path = CreateTestGroupFile("");
    map<string, string> args;
    args["group"] = "foo";
    args["gid"] = "8888";
    args["test_etcGroupPath"] = path;
    auto result = AuditEnsureGroupIsOnlyGroupWith(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureGroupIsOnlyGroupWithTest, NoParameter)
{
    auto path = CreateTestGroupFile(string("foo"), string("x"), 8888);
    map<string, string> args;
    args["group"] = "foo";
    args["test_etcGroupPath"] = path;
    auto result = AuditEnsureGroupIsOnlyGroupWith(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGroupIsOnlyGroupWithTest, SingleGID)
{
    auto path = CreateTestGroupFile(string("foo"), string("x"), 8888);
    map<string, string> args;
    args["group"] = "foo";
    args["gid"] = "8888";
    args["test_etcGroupPath"] = path;
    auto result = AuditEnsureGroupIsOnlyGroupWith(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGroupIsOnlyGroupWithTest, DuplicatedGID)
{
    auto path = CreateTestGroupFile("foo:x:8888:\nbar:x:8888:");
    map<string, string> args;
    args["group"] = "foo";
    args["gid"] = "8888";
    args["test_etcGroupPath"] = path;
    auto result = AuditEnsureGroupIsOnlyGroupWith(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
