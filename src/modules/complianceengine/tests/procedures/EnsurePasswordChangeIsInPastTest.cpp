// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsurePasswordChangeIsInPast;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsurePasswordChangeIsInPastTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    CompactListFormatter mFormatter;
    string mTempDir;

    void SetUp() override
    {
        mIndicators.Push("EnsurePasswordChangeIsInPast");
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

TEST_F(EnsurePasswordChangeIsInPastTest, SingleUser_Compliant_1)
{
    map<string, string> args;
    auto path = CreateTestShadowFile("testuser", string("$y$"), 1, 2, 3, 4, 5, 6);
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsurePasswordChangeIsInPast(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsurePasswordChangeIsInPastTest, SingleUser_Compliant_2)
{
    map<string, string> args;
    auto today = time(nullptr) / (24 * 3600); // Set to today
    auto path = CreateTestShadowFile("testuser", string("$y$"), today, 2, 3, 4, 5, 6);
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsurePasswordChangeIsInPast(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsurePasswordChangeIsInPastTest, SingleUser_NonCompliant_1)
{
    map<string, string> args;
    auto tomorrow = time(nullptr) / (24 * 3600) + 1; // Set to tomorrow
    auto path = CreateTestShadowFile("testuser", string("$y$"), tomorrow, 2, 3, 4, 5, 6);
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsurePasswordChangeIsInPast(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsurePasswordChangeIsInPastTest, SingleUser_NonCompliant_2)
{
    map<string, string> args;
    string contents = "user1:$y$:99999:2:3:4:5:6:";
    contents += "\nuser2:$y$:99999:2:3:4:5:6:";
    contents += "\nuser3:$y$:99999:2:3:4:5:6:";
    contents += "\nuser4:$y$:99999:2:3:4:5:6:";
    contents += "\nuser5:$y$:99999:2:3:4:5:6:";
    contents += "\nuser6:$y$:99999:2:3:4:5:6:";
    contents += "\nuser7:$y$:99999:2:3:4:5:6:";
    contents += "\nuser8:$y$:99999:2:3:4:5:6:";
    auto path = CreateTestShadowFile(std::move(contents));
    args["test_etcShadowPath"] = path;
    auto result = AuditEnsurePasswordChangeIsInPast(args, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    auto formatted = mFormatter.Format(mIndicators);
    ASSERT_TRUE(formatted.HasValue());
    auto lineCount = std::count(formatted.Value().begin(), formatted.Value().end(), '\n') + 1;
    ASSERT_EQ(lineCount, 7); // Expecting 5 non-compliant users + some formatting artifacts
}
