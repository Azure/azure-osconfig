// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Regex.h"

#include "Base64.h"
#include "Result.h"

#include <gtest/gtest.h>

using compliance::Error;
using compliance::Regex;
using compliance::Result;
class RegexTest : public ::testing::Test
{
};

TEST_F(RegexTest, EmptyString)
{
    auto regex = Regex::Compile("^$");
    ASSERT_TRUE(regex.HasValue());
    EXPECT_FALSE(regex->Match("test"));
    EXPECT_TRUE(regex->Match(""));
}

TEST_F(RegexTest, Lookbehind)
{
    auto regex = Regex::Compile(R"((?<=\d)abc)");
    ASSERT_TRUE(regex.HasValue());

    EXPECT_TRUE(regex->Match("1abc"));
    EXPECT_FALSE(regex->Match("abc"));
}

TEST_F(RegexTest, NamedCapturingGroup)
{
    auto regex = Regex::Compile(R"((?<name>\w+))");
    ASSERT_TRUE(regex.HasValue());

    EXPECT_TRUE(regex->Match("hello"));
    EXPECT_FALSE(regex->Match("."));
}

TEST_F(RegexTest, ConditionalPatterns)
{
    auto regex = Regex::Compile(R"((?(?=\d)\d{2}|[a-b]{2}))");
    ASSERT_TRUE(regex.HasValue());

    EXPECT_FALSE(regex->Match("1a"));
    EXPECT_TRUE(regex->Match("12"));
    EXPECT_TRUE(regex->Match("ab"));
    EXPECT_FALSE(regex->Match("a1"));
}
