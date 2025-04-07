// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "ProcedureMap.h"

#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using compliance::AuditFileRegexMatch;
using compliance::Error;
using compliance::Result;

#include <iostream>
class FileRegexMatchTest : public ::testing::Test
{
protected:
    char mTempfile[PATH_MAX] = "/tmp/fileRegexMatchTest.XXXXXX";
    int mTempfileFd = -1;
    std::map<std::string, std::string> mArgs;
    std::ostringstream mLogstream;

    void SetUp() override
    {
        sprintf(mTempfile, "/tmp/fileRegexMatchTest.XXXXXX");
        mTempfileFd = mkstemp(mTempfile);
        ASSERT_TRUE(mTempfileFd != -1);
    }

    void TearDown() override
    {
        remove(mTempfile);
        close(mTempfileFd);
    }

    void FillTempfile(const std::string& content)
    {
        std::ofstream file(mTempfile);
        file << content;
    }
};

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_1)
{
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_2)
{
    mArgs["filename"] = mTempfile;
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_3)
{
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "test";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    if (result.HasValue())
    {
        std::cerr << mLogstream.str() << std::endl;
        std::cerr << "Result: " << result.Value() << std::endl;
    }
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_4)
{
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "test"; // invalid match operation value
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(FileRegexMatchTest, Audit_InvalidArguments_5)
{
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "(?i)"; // invalid regex pattern
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_EmptyFile_1)
{
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(FileRegexMatchTest, Audit_Match_1)
{
    FillTempfile("test");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Match_2)
{
    FillTempfile("tests");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "test";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Match_3)
{
    FillTempfile("test");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "tests";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Match_4)
{
    FillTempfile("test");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "te.t";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Match_5)
{
    FillTempfile("test");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = "^te.t$";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Match_6)
{
    FillTempfile(" \ttesting");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^[[:space:]]*te[a-z]t.*$)";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_CaseInsensitive_1)
{
    FillTempfile(" \ttesTing");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^[[:space:]]*Te[a-z]t.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["caseSensitive"] = "false";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_State_1)
{
    FillTempfile("key=foo");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=foo$)";
    mArgs["stateOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_State_2)
{
    FillTempfile("key=foo");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=bar$)";
    mArgs["stateOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Multiline_Match_2)
{
    FillTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_2)
{
    FillTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=bar$)";
    mArgs["stateOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_3)
{
    FillTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=(foo|bar|baz)$)";
    mArgs["stateOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(FileRegexMatchTest, Audit_Multiline_State_4)
{
    FillTempfile("key=foo\nkey=bar\nkey=baz");
    mArgs["filename"] = mTempfile;
    mArgs["matchPattern"] = R"(^key=.*$)";
    mArgs["matchOperation"] = "pattern match";
    mArgs["statePattern"] = R"(^key=(foo|bar)$)";
    mArgs["stateOperation"] = "pattern match";
    auto result = AuditFileRegexMatch(mArgs, mLogstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value());
}
