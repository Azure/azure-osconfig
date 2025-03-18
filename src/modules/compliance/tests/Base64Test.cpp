// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Base64.h"

#include "Result.h"

#include <gtest/gtest.h>

using compliance::Base64Decode;
using compliance::Error;
using compliance::Result;

class Base64Test : public ::testing::Test
{
};

TEST_F(Base64Test, InvalidLength)
{
    Result<std::string> result = Base64Decode("abc");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid base64 length");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(Base64Test, InvalidCharacter)
{
    Result<std::string> result = Base64Decode("abc$");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid base64 character");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(Base64Test, ValidBase64WithoutPadding)
{
    Result<std::string> result = Base64Decode("SGVsbG8gV29ybGQh");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), "Hello World!");
}

TEST_F(Base64Test, ValidBase64WithOnePadding)
{
    Result<std::string> result = Base64Decode("SGVsbG8gV29ybGQ=");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), "Hello World");
}

TEST_F(Base64Test, ValidBase64WithTwoPadding)
{
    Result<std::string> result = Base64Decode("SGVsbG8gV29ybA==");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), "Hello Worl");
}

TEST_F(Base64Test, InvalidThreePadding)
{
    Result<std::string> result = Base64Decode("SGVsbG8gd29yb===");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid base64");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(Base64Test, ValidBase64WithSpecials)
{
    Result<std::string> result = Base64Decode("SGVsbG8gV29ybGQgZm8/YmE+");
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), "Hello World fo?ba>");
}
