// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define USE_REGEX_FALLBACK 1
#include <RegexFallback.h>
#include <cstring>
#include <gtest/gtest.h>

class RegexFallbackTest : public ::testing::Test
{
};

TEST_F(RegexFallbackTest, NoMatch)
{
    std::string target = "This is a test string";
    std::string pattern = "notfound";

    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    EXPECT_FALSE(match.ready());

    bool result = regex_search(target, match, r);
    EXPECT_FALSE(result);
    ASSERT_TRUE(match.ready());
    EXPECT_EQ(match.size(), 0);
}

TEST_F(RegexFallbackTest, Match)
{
    std::string target = "This is a test string";
    std::string pattern = "test";

    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;

    bool result = regex_search(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
    ASSERT_EQ(match.size(), 1);
    EXPECT_EQ(match[0].matched, true);
    EXPECT_EQ(match[0].length(), 4);
}

TEST_F(RegexFallbackTest, MatchWithSubMatches_1)
{
    std::string target = "This is a test string";
    std::string pattern = "(test)";
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_search(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
    ASSERT_EQ(match.size(), 2);
    EXPECT_EQ(match[0].matched, true);
    EXPECT_EQ(match[0].length(), std::strlen("test"));
    EXPECT_EQ(match[1].matched, true);
    EXPECT_EQ(match[1].length(), std::strlen("test"));
}

TEST_F(RegexFallbackTest, MatchWithSubMatches_2)
{
    std::string target = "This is a test string";
    std::string pattern = "(test) (string)";
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_search(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
    ASSERT_EQ(match.size(), 3);
    EXPECT_EQ(match[0].matched, true);
    EXPECT_EQ(match[0].length(), std::strlen("test string"));
    EXPECT_EQ(match[1].matched, true);
    EXPECT_EQ(match[1].length(), std::strlen("test"));
    EXPECT_EQ(match[2].matched, true);
    EXPECT_EQ(match[2].length(), std::strlen("string"));
}

TEST_F(RegexFallbackTest, MatchWithSubMatches_3)
{
    std::string target = "This is a test string";
    std::string pattern = "((test) (string))";
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_search(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
    ASSERT_EQ(match.size(), 4);
    EXPECT_EQ(match[0].matched, true);
    EXPECT_EQ(match[0].length(), std::strlen("test string"));
    EXPECT_EQ(match[1].matched, true);
    EXPECT_EQ(match[1].length(), std::strlen("test string"));
    EXPECT_EQ(match[2].matched, true);
    EXPECT_EQ(match[2].length(), std::strlen("test"));
    EXPECT_EQ(match[3].matched, true);
    EXPECT_EQ(match[3].length(), std::strlen("string"));
    EXPECT_EQ(match[100].matched, false);
    EXPECT_EQ(match[100].length(), 0);
}

TEST_F(RegexFallbackTest, RangeLoop)
{
    std::string target = "This is a test string";
    std::string pattern = "((test) (string))";
    std::string output;
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_search(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
    for (const auto& m : match)
    {
        output += m.str();
    }
    EXPECT_EQ(output, "test stringtest stringteststring");
}

TEST_F(RegexFallbackTest, PrefixAndSuffix)
{
    std::string target = "This is a test string?";
    std::string pattern = "((test) (string))";
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_search(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
    EXPECT_EQ(match.prefix(), "This is a ");
    EXPECT_EQ(match.suffix(), "?");
}

TEST_F(RegexFallbackTest, RegexMatch_1)
{
    std::string target = "This is a test string?";
    std::string pattern = "((test) (string))";
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_match(target, match, r);
    EXPECT_FALSE(result);
    EXPECT_FALSE(match.ready());
}

TEST_F(RegexFallbackTest, RegexMatch_2)
{
    std::string target = "This is a test string?";
    std::string pattern = "This is a ((test) (string))";
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_match(target, match, r);
    EXPECT_FALSE(result);
    EXPECT_FALSE(match.ready());
}

TEST_F(RegexFallbackTest, RegexMatch_3)
{
    std::string target = "This is a test string?";
    std::string pattern = R"(This is a ((test) (string))\?)";
    auto r = regex(pattern, std::regex_constants::extended);
    smatch match;
    bool result = regex_match(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
}

TEST_F(RegexFallbackTest, RegexMatch_4)
{
    std::string target = "account\t[success=1 new_authtok_reqd=done default=ignore]\tpam_unix.so ";
    std::string pattern = R"(^[ \t]*account[ \t]+[^#\n\r]+[ \t]+pam_unix\.so\b)";
    auto r = regex(pattern);
    smatch match;
    bool result = regex_search(target, match, r);
    EXPECT_TRUE(result);
    ASSERT_TRUE(match.ready());
}
