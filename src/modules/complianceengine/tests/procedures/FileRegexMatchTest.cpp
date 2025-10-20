// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MockContext.h"

#include <FileRegexMatch.h>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using ComplianceEngine::AuditFileRegexMatch;
using ComplianceEngine::AuditFileRegexMatchParams;
using ComplianceEngine::Behavior;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::Error;
using ComplianceEngine::IgnoreCase;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Operation;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::string;

class FileRegexMatchTest : public ::testing::Test
{
protected:
    char mTempdir[PATH_MAX] = "/tmp/FileRegexMatchTest.XXXXXX";
    MockContext mContext;
    IndicatorsTree mIndicators;
    std::vector<string> mTempfiles;

    void SetUp() override
    {
        mIndicators.Push("FileRegexMatch");
        ASSERT_NE(mkdtemp(mTempdir), nullptr);
    }

    void TearDown() override
    {
        for (const auto& file : mTempfiles)
        {
            remove(file.c_str());
        }
        remove(mTempdir);
    }

    void MakeTempfile(const string& content)
    {
        string filename = mTempdir;
        if (mTempfiles.empty())
        {
            filename += "/1";
            mTempfiles.push_back(filename);
        }
        else
        {
            const auto& last = mTempfiles.back();
            auto index = last.find_last_of('/');
            filename = last.substr(0, index) + "/" + std::to_string(std::stoi(last.substr(index + 1)) + 1);
            mTempfiles.push_back(filename);
        }

        std::ofstream file(filename);
        file << content;
    }
};

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_1)
{
    AuditFileRegexMatchParams params;
    MakeTempfile("test");
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = "(?i)"; // invalid regex pattern
    params.matchOperation = Operation::Match;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_EmptyFile_1)
{
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = "test";
    params.matchOperation = Operation::Match;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_1)
{
    MakeTempfile("test");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = "test";
    params.matchOperation = Operation::Match;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_2)
{
    MakeTempfile("tests");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = "test";
    params.matchOperation = Operation::Match;
    params.behavior = Behavior::NoneExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_3)
{
    MakeTempfile("test");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = "tests";
    params.matchOperation = Operation::Match;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_4)
{
    MakeTempfile("test");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = "te.t";
    params.matchOperation = Operation::Match;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_5)
{
    MakeTempfile("test");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = "^te.t$";
    params.matchOperation = Operation::Match;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_6)
{
    MakeTempfile(" \ttesting");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^[[:space:]]*te[a-z]t.*$)";
    params.matchOperation = Operation::Match;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_CaseInsensitive_1)
{
    MakeTempfile(" \ttesTing");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^[[:space:]]*Te[a-z]t.*$)";
    params.matchOperation = Operation::Match;
    params.ignoreCase = ComplianceEngine::IgnoreCase::MatchPattern;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_State_1)
{
    MakeTempfile("key=foo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.statePattern = string(R"(^key=foo$)");
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_State_2_CaseInsensitve)
{
    MakeTempfile("key=foo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.statePattern = string(R"(^key=FoO$)");
    params.behavior = Behavior::AllExist;
    params.ignoreCase = IgnoreCase::StatePattern;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_State_2_CaseInsensitveBoth)
{
    MakeTempfile("key=foo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^Key=.*$)";
    params.statePattern = string(R"(^Key=FoO$)");
    params.behavior = Behavior::AllExist;
    params.ignoreCase = IgnoreCase::Both;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_State_2_CaseInsensitveBothDiffetnArg)
{
    MakeTempfile("key=foo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^Key=.*$)";
    params.statePattern = string(R"(^Key=FoO$)");
    params.behavior = Behavior::AllExist;
    params.ignoreCase = IgnoreCase::Both;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
TEST_F(FileRegexMatchTest, Audit_State_2)
{
    MakeTempfile("key=foo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.statePattern = string(R"(^key=bar$)");
    params.stateOperation = Operation::Match;
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_State_3)
{
    MakeTempfile("key=foo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.statePattern = string(R"(^key=bar$)");
    params.stateOperation = Operation::Match;
    params.behavior = Behavior::NoneExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_State_4)
{
    MakeTempfile("key=bar\nkey=foo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.statePattern = string(R"(^key=foo$)");
    params.stateOperation = Operation::Match;
    params.behavior = Behavior::NoneExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_Match_1)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_Match_2)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz\nky=typo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.behavior = Behavior::AtLeastOneExists;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_Match_3)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz\nky=typo");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_1)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.statePattern = string(R"(^key=bar$)");
    params.stateOperation = Operation::Match;
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_2)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.statePattern = string(R"(^key=(foo|bar|baz)$)");
    params.stateOperation = Operation::Match;
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_4)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.matchOperation = Operation::Match;
    params.statePattern = string(R"(^key=(foo|bar)$)");
    params.stateOperation = Operation::Match;
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_1)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchPattern = R"(^key=.*$)";
    params.statePattern = string(R"(^key=(foo|bar)$)");
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_2)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("2"); // no such file
    params.matchPattern = R"(^key=.*$)";
    params.statePattern = string(R"(^key=(foo|bar)$)");
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_3)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex(".*");
    params.matchPattern = R"(^key=.*$)";
    params.statePattern = string(R"(^key=(foo|bar)$)");
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    CompactListFormatter formatter;
    auto payload = formatter.Format(mIndicators);
    ASSERT_TRUE(payload.HasValue());
    std::cerr << "Payload: " << payload.Value() << std::endl;
    EXPECT_NE(payload.Value().find("[NonCompliant] At least one file did not match the pattern"), string::npos);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_4)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("2");
    params.matchPattern = R"(^key=.*$)";
    params.statePattern = string(R"(^key=(foo|bar|baz)$)");
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_5)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex(".*");
    params.matchPattern = R"(^key=.*$)";
    params.statePattern = string(R"(^key=(foo|bar|baz)$)");
    params.behavior = Behavior::AtLeastOneExists;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_6)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("2");
    params.matchPattern = R"(^key=(.*)$)";
    params.statePattern = string(R"(^(foo|bar|baz)$)"); // Unlike the previous test, this matches against 'foo', 'bar', and 'baz'
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_7)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("2");
    params.matchPattern = R"(^key=(.*)$)";
    params.statePattern = string(R"(^key=(foo|bar|baz)$)"); // This won't work now as we match against 'foo', 'bar', and 'baz'
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_8)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("2");
    params.matchPattern = R"(^(key=(.*))$)";
    params.statePattern = string(R"(^key=(foo|bar|baz)$)"); // This should work again as we added a capturing group for the full key=value
    params.behavior = Behavior::AllExist;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_TestPattern)
{
    MakeTempfile(
        "# here are the per-package modules (the \"Primary\" block)\naccount\t[success=1 new_authtok_reqd=done default=ignore]\tpam_unix.so \n# here's "
        "the fallback if no module succeeds\n");
    AuditFileRegexMatchParams params;
    params.path = mTempdir;
    params.filenamePattern = regex("1");
    params.matchOperation = Operation::Match;
    params.matchPattern = R"(^[ \t]*account[ \t]+[^#\n\r]+[ \t]+pam_unix\.so\b)";
    params.behavior = Behavior::AtLeastOneExists;
    auto result = AuditFileRegexMatch(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}
