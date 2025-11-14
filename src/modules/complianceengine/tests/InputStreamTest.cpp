// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <InputStream.h>
#include <MockContext.h>
#include <gtest/gtest.h>

using ComplianceEngine::Error;
using ComplianceEngine::InputStream;
using ComplianceEngine::Result;
using std::string;

class InputStreamTest : public ::testing::Test
{
protected:
    MockContext mContext;
};

TEST_F(InputStreamTest, DoesNotExist)
{
    auto result = InputStream::Open("nonexistentfile", mContext);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, ENOENT);
}

TEST_F(InputStreamTest, EmptyFile)
{
    const auto filename = mContext.MakeTempfile("");
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());

    // Not yet at end as we haven't read anything yet.
    EXPECT_TRUE(result->Good());
    EXPECT_FALSE(result->AtEnd());

    // This should return an empty line
    auto line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string());

    // Subsequent reads should fail as we've reached EOF
    EXPECT_FALSE(result->Good());
    EXPECT_TRUE(result->AtEnd());
    line = result->ReadLine();
    ASSERT_FALSE(line.HasValue());
    EXPECT_EQ(line.Error().code, EBADFD);
}

TEST_F(InputStreamTest, SingleLine)
{
    const auto filename = mContext.MakeTempfile("foo\n");
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());

    // Not yet at end as we haven't read anything yet.
    EXPECT_TRUE(result->Good());
    EXPECT_FALSE(result->AtEnd());

    // This should return a line with 'foo' contents
    auto line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string("foo"));

    // Not yet at end as we have not reached EOF state yet
    EXPECT_TRUE(result->Good());
    EXPECT_FALSE(result->AtEnd());

    // This should return an empty line
    line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string());

    // Subsequent reads should fail as we've reached EOF
    EXPECT_FALSE(result->Good());
    EXPECT_TRUE(result->AtEnd());
    line = result->ReadLine();
    ASSERT_FALSE(line.HasValue());
    EXPECT_EQ(line.Error().code, EBADFD);
}

TEST_F(InputStreamTest, MultipleLines)
{
    const auto filename = mContext.MakeTempfile("foo \n bar \r\nbaz");
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());

    // Not yet at end as we haven't read anything yet.
    EXPECT_TRUE(result->Good());
    EXPECT_FALSE(result->AtEnd());

    // This should return a line with 'foo' contents
    auto line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string("foo "));

    // Not yet at end as we have not reached EOF state yet
    EXPECT_TRUE(result->Good());
    EXPECT_FALSE(result->AtEnd());

    // This should return a line with 'bar' contents.
    // It will include the \r as well as it runs on Linux (std::getline behavior)
    line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string(" bar \r"));

    // This should return a line with 'baz' contents
    line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string("baz"));

    // We've reached the EOF as there's no line ending at the end of input.
    EXPECT_FALSE(result->Good());
    EXPECT_TRUE(result->AtEnd());
    line = result->ReadLine();
    ASSERT_FALSE(line.HasValue());
    EXPECT_EQ(line.Error().code, EBADFD);
}

TEST_F(InputStreamTest, Range_MultipleLines)
{
    const auto filename = mContext.MakeTempfile("foo\nbar\nbaz\n\n");
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());

    string test;
    int counter = 0;
    // Test the LinesRange with iterator for range-based for loops use-case
    for (auto line : result->Lines())
    {
        ASSERT_TRUE(line.HasValue());
        test += line.Value();
        ++counter;
    }

    EXPECT_FALSE(result->Good());
    EXPECT_EQ(test, string("foobarbaz"));
    EXPECT_EQ(counter, 5);
    EXPECT_FALSE(result->Good());
    EXPECT_TRUE(result->AtEnd());
}

TEST_F(InputStreamTest, Mocking)
{
    const auto filename = mContext.MakeTempfile("foo");
    mContext.SetSpecialFilePath("/etc/passwd", filename);

    // The /etc/passwd file should be masked by the tempfile we've just created
    auto result = InputStream::Open("/etc/passwd", mContext);
    ASSERT_TRUE(result.HasValue());
    auto line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string("foo"));
}

TEST_F(InputStreamTest, LimitsHandling_1)
{
    const auto input = string();
    const auto filename = mContext.MakeTempfile(input);
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());
    auto line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string());
    EXPECT_EQ(result->BytesRead(), input.size());
}

TEST_F(InputStreamTest, LimitsHandling_2)
{
    const auto input = string("foo");
    const auto filename = mContext.MakeTempfile(input);
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());
    auto line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string("foo"));
    EXPECT_EQ(result->BytesRead(), input.size());
}

TEST_F(InputStreamTest, LimitsHandling_3)
{
    const auto input = string("foo\n");
    const auto filename = mContext.MakeTempfile(input);
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());
    auto line = result->ReadLine();
    ASSERT_TRUE(line.HasValue());
    EXPECT_EQ(line.Value(), string("foo"));
    EXPECT_EQ(result->BytesRead(), input.size());
}

TEST_F(InputStreamTest, LimitsHandling_4)
{
    const auto input = string("foo\n\nbar");
    const auto filename = mContext.MakeTempfile(input);
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());
    for (auto line : result->Lines())
    {
        ASSERT_TRUE(line.HasValue());
    }
    EXPECT_EQ(result->BytesRead(), input.size());
}

TEST_F(InputStreamTest, LimitsHandling_6)
{
    const auto input = string(InputStream::cMaxReadSize, 'x');
    const auto filename = mContext.MakeTempfile(input);
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());
    std::size_t counter = 0;
    for (auto line : result->Lines())
    {
        ASSERT_TRUE(line.HasValue());
        ++counter;
    }
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(result->BytesRead(), input.size());
}

TEST_F(InputStreamTest, LimitsHandling_7)
{
    constexpr auto limit = InputStream::cMaxReadSize;
    string input;
    for (std::size_t i = 0; i < 1023; ++i)
    {
        input += string(InputStream::cMaxReadSize / 1024, 'x');
        input += '\n';
    }
    const auto filename = mContext.MakeTempfile(input);
    auto result = InputStream::Open(filename, mContext);
    ASSERT_TRUE(result.HasValue());
    std::size_t counter = 0;
    for (auto line : result->Lines())
    {
        ASSERT_TRUE(line.HasValue());
        ++counter;
    }
    // +1 for the trailing empty line
    EXPECT_EQ(counter, 1024);
    EXPECT_TRUE(result->BytesRead() < limit);
}

TEST_F(InputStreamTest, LimitsHandling_8)
{
    constexpr auto limit = InputStream::cMaxReadSize;
    string input;
    for (std::size_t i = 0; i < 1024; ++i)
    {
        input += string(InputStream::cMaxReadSize / 1024, 'x');
        input += '\n';
    }
    const auto filename = mContext.MakeTempfile(input);
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = InputStream::Open("/etc/passwd", mContext);
    ASSERT_TRUE(result.HasValue());
    std::size_t counter = 0;
    for (auto line : result->Lines())
    {
        ASSERT_TRUE(line.HasValue());
        ++counter;
    }
    EXPECT_EQ(counter, 1024);
    // We exceed the limit here, but won't be able to read next time
    EXPECT_TRUE(result->BytesRead() > limit);

    // Limit reached
    auto line = result->ReadLine();
    ASSERT_FALSE(line.HasValue());
    EXPECT_EQ(line.Error().code, E2BIG);
}
