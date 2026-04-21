// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "StringTools.h"

#include <gtest/gtest.h>

using ComplianceEngine::EscapeForShell;
using ComplianceEngine::TrimWhiteSpaces;

class StringToolsTest : public ::testing::Test
{
};

// Tests for EscapeForShell

TEST_F(StringToolsTest, EscapeForShell_EmptyString)
{
    EXPECT_EQ("", EscapeForShell(""));
}

TEST_F(StringToolsTest, EscapeForShell_NormalString)
{
    EXPECT_EQ("hello", EscapeForShell("hello"));
    EXPECT_EQ("hello world", EscapeForShell("hello world"));
    EXPECT_EQ("/path/to/file", EscapeForShell("/path/to/file"));
}

TEST_F(StringToolsTest, EscapeForShell_Backslashes)
{
    // Backslashes should be escaped
    EXPECT_EQ("\\\\", EscapeForShell("\\"));
    EXPECT_EQ("path\\\\to\\\\file", EscapeForShell("path\\to\\file"));
    EXPECT_EQ("\\\\n", EscapeForShell("\\n"));
    EXPECT_EQ("\\\\t", EscapeForShell("\\t"));
}

TEST_F(StringToolsTest, EscapeForShell_DoubleQuotes)
{
    // Double quotes should be escaped
    EXPECT_EQ("\\\"", EscapeForShell("\""));
    EXPECT_EQ("say \\\"hello\\\"", EscapeForShell("say \"hello\""));
    EXPECT_EQ("\\\"quoted\\\"", EscapeForShell("\"quoted\""));
}

TEST_F(StringToolsTest, EscapeForShell_Backticks)
{
    // Backticks should be escaped (command substitution)
    EXPECT_EQ("\\`", EscapeForShell("`"));
    EXPECT_EQ("\\`whoami\\`", EscapeForShell("`whoami`"));
    EXPECT_EQ("\\`id\\`", EscapeForShell("`id`"));
}

TEST_F(StringToolsTest, EscapeForShell_DollarSign)
{
    // Dollar signs should be escaped (variable expansion)
    EXPECT_EQ("\\$", EscapeForShell("$"));
    EXPECT_EQ("\\$HOME", EscapeForShell("$HOME"));
    EXPECT_EQ("\\$(whoami)", EscapeForShell("$(whoami)"));
    EXPECT_EQ("\\${USER}", EscapeForShell("${USER}"));
}

TEST_F(StringToolsTest, EscapeForShell_MultipleSpecialCharacters)
{
    // Multiple special characters should all be escaped
    EXPECT_EQ("\\\"\\$HOME\\\"", EscapeForShell("\"$HOME\""));
    EXPECT_EQ("\\`echo \\$USER\\`", EscapeForShell("`echo $USER`"));
    EXPECT_EQ("\\\\\\\"\\\\\\`\\$", EscapeForShell("\\\"\\`$"));
}

TEST_F(StringToolsTest, EscapeForShell_CommandInjectionPatterns)
{
    // Command injection patterns should be properly escaped
    EXPECT_EQ("; rm -rf /", EscapeForShell("; rm -rf /"));               // Semicolon not escaped
    EXPECT_EQ("&& malicious", EscapeForShell("&& malicious"));           // && not escaped
    EXPECT_EQ("| cat /etc/passwd", EscapeForShell("| cat /etc/passwd")); // Pipe not escaped

    // But these dangerous patterns with shell variables/commands should be escaped
    EXPECT_EQ("\\$(cat /etc/passwd)", EscapeForShell("$(cat /etc/passwd)"));
    EXPECT_EQ("\\`cat /etc/passwd\\`", EscapeForShell("`cat /etc/passwd`"));
}

TEST_F(StringToolsTest, EscapeForShell_MixedContent)
{
    // Real-world examples with mixed content
    EXPECT_EQ("hostname\\\"test\\\"", EscapeForShell("hostname\"test\""));
    EXPECT_EQ("user\\$name", EscapeForShell("user$name"));
    EXPECT_EQ("path\\\\with\\\\slashes", EscapeForShell("path\\with\\slashes"));
}

// Tests for TrimWhiteSpaces

TEST_F(StringToolsTest, TrimWhiteSpaces_EmptyString)
{
    EXPECT_EQ("", TrimWhiteSpaces(""));
}

TEST_F(StringToolsTest, TrimWhiteSpaces_NoWhitespace)
{
    EXPECT_EQ("hello", TrimWhiteSpaces("hello"));
}

TEST_F(StringToolsTest, TrimWhiteSpaces_LeadingWhitespace)
{
    EXPECT_EQ("hello", TrimWhiteSpaces("  hello"));
    EXPECT_EQ("hello", TrimWhiteSpaces("\thello"));
    EXPECT_EQ("hello", TrimWhiteSpaces("\nhello"));
    EXPECT_EQ("hello", TrimWhiteSpaces("  \t\nhello"));
}

TEST_F(StringToolsTest, TrimWhiteSpaces_TrailingWhitespace)
{
    EXPECT_EQ("hello", TrimWhiteSpaces("hello  "));
    EXPECT_EQ("hello", TrimWhiteSpaces("hello\t"));
    EXPECT_EQ("hello", TrimWhiteSpaces("hello\n"));
    EXPECT_EQ("hello", TrimWhiteSpaces("hello  \t\n"));
}

TEST_F(StringToolsTest, TrimWhiteSpaces_BothEnds)
{
    EXPECT_EQ("hello", TrimWhiteSpaces("  hello  "));
    EXPECT_EQ("hello world", TrimWhiteSpaces("  hello world  "));
    EXPECT_EQ("hello", TrimWhiteSpaces("\t\n hello \n\t"));
}

TEST_F(StringToolsTest, TrimWhiteSpaces_OnlyWhitespace)
{
    EXPECT_EQ("", TrimWhiteSpaces("   "));
    EXPECT_EQ("", TrimWhiteSpaces("\t\n"));
    EXPECT_EQ("", TrimWhiteSpaces("  \t  \n  "));
}

TEST_F(StringToolsTest, TrimWhiteSpaces_InternalWhitespace)
{
    // Internal whitespace should be preserved
    EXPECT_EQ("hello  world", TrimWhiteSpaces("  hello  world  "));
    EXPECT_EQ("hello\tworld", TrimWhiteSpaces("hello\tworld"));
}
