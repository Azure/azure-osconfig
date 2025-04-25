// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using compliance::AuditFileRegexMatch;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::Result;
using compliance::Status;

class FileRegexMatchTest : public ::testing::Test
{
protected:
    char mTempdir[PATH_MAX] = "/tmp/FileRegexMatchTest.XXXXXX";
    std::map<std::string, std::string> mArgs;
    MockContext mContext;
    IndicatorsTree mIndicators;
    std::vector<std::string> mTempfiles;

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

    void MakeTempfile(const std::string& content)
    {
        std::string filename = mTempdir;
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
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_2)
{
    mArgs["path"] = mTempdir;
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_3)
{
    mArgs["path"] = "/foobarbaztest";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_4)
{
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_5)
{
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "test"; // invalid match operation value
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_6)
{
    MakeTempfile("test");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "(?i)"; // invalid regex pattern
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_7)
{
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "test";
    mArgs["stateOperation"] = "test"; // invalid state operation value
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_EmptyFile_1)
{
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_1)
{
    MakeTempfile("test");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_2)
{
    MakeTempfile("tests");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_3)
{
    MakeTempfile("test");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "tests";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_4)
{
    MakeTempfile("test");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "te.t";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_5)
{
    MakeTempfile("test");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = "^te.t$";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Match_6)
{
    MakeTempfile(" \ttesting");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^[[:space:]]*te[a-z]t.*$)";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_CaseInsensitive_1)
{
    MakeTempfile(" \ttesTing");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^[[:space:]]*Te[a-z]t.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["caseSensitive"] = "false";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_State_1)
{
    MakeTempfile("key=foo");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["statePattern"] = R"(^key=foo$)";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_State_2)
{
    MakeTempfile("key=foo");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=bar$)";
    mArgs["stateOperation"] = "pattern match";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_State_3)
{
    MakeTempfile("key=foo");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=bar$)";
    mArgs["stateOperation"] = "pattern match";
    mArgs["behavior"] = "none_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_State_4)
{
    MakeTempfile("key=foo");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=foo$)";
    mArgs["stateOperation"] = "pattern match";
    mArgs["behavior"] = "none_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_Match_1)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_Match_2)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz\nky=typo");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["behavior"] = "at_least_one_exists";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_Match_3)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz\nky=typo");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_1)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=bar$)";
    mArgs["stateOperation"] = "pattern match";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_2)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=(foo|bar|baz)$)";
    mArgs["stateOperation"] = "pattern match";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_4)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=(foo|bar)$)";
    mArgs["stateOperation"] = "pattern match";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_1)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "1";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["statePattern"] = R"(^key=(foo|bar)$)";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_2)
{
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "2"; // no such file
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["statePattern"] = R"(^key=(foo|bar)$)";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_3)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = ".*";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["statePattern"] = R"(^key=(foo|bar)$)";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    compliance::CompactListFormatter formatter;
    auto payload = formatter.Format(mIndicators);
    ASSERT_TRUE(payload.HasValue());
    EXPECT_NE(payload.Value().find("[NonCompliant] pattern '^key=.*$' did not match line 1"), std::string::npos);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_4)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "2";
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["statePattern"] = R"(^key=(foo|bar|baz)$)";
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_5)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "2";
    mArgs["matchPattern"] = R"(^key=(.*)$)";
    mArgs["statePattern"] = R"(^(foo|bar|baz)$)"; // Unlike the previous test, this matches against 'foo', 'bar', and 'baz'
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_6)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "2";
    mArgs["matchPattern"] = R"(^key=(.*)$)";
    mArgs["statePattern"] = R"(^key=(foo|bar|baz)$)"; // This won't work now as we match against 'foo', 'bar', and 'baz'
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(FileRegexMatchTest, Audit_FilenamePattern_7)
{
    MakeTempfile("nothing important here");
    MakeTempfile("key=foo\nkey=bar\nkey=baz");
    MakeTempfile("nothing important here as well");
    mArgs["path"] = mTempdir;
    mArgs["filenamePattern"] = "2";
    mArgs["matchPattern"] = R"(^(key=(.*))$)";
    mArgs["statePattern"] = R"(^key=(foo|bar|baz)$)"; // This should work again as we added a capturing group for the full key=value
    mArgs["behavior"] = "all_exist";
    auto result = AuditFileRegexMatch(mArgs, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}
