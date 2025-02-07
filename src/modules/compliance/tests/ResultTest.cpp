// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include "Result.h"

using compliance::Result;
using compliance::Error;

class ResultTest : public ::testing::Test
{
};

TEST_F(ResultTest, ErrorContructor)
{
    Result<int> result(Error("error"));
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error().message, std::string("error"));
}

TEST_F(ResultTest, ValueContructor)
{
    Result<int> result(42);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), 42);

    result = Result<int>{Error("error")};
    ASSERT_FALSE(result.has_value());
}

TEST_F(ResultTest, CopyContructor)
{
    Result<int> result1(42);
    Result<int> result2(result1);
    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2.value(), 42);
}

TEST_F(ResultTest, MoveContructor)
{
    Result<int> result1(42);
    Result<int> result2(std::move(result1));
    ASSERT_FALSE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2.value(), 42);
}

TEST_F(ResultTest, CopyAssignment)
{
    Result<int> result1(42);
    auto result2 = result1;
    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2.value(), 42);
}

TEST_F(ResultTest, MoveAssignment)
{
    Result<int> result1(42);
    Result<int> result2 = std::move(result1);
    ASSERT_FALSE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2.value(), 42);
}

TEST_F(ResultTest, ValueAssignment)
{
    Result<int> result(Error("error"));
    result = 42;
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), 42);
}

TEST_F(ResultTest, ValueReference)
{
    Result<int> result(42);
    result.value() = 43;
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), 43);
}

TEST_F(ResultTest, ErrorReference)
{
    auto result = Result<int>(Error("error"));
    result.error().message = "ERROR";
    ASSERT_EQ(result.error().message, "ERROR");
}

TEST_F(ResultTest, BoolConversion)
{
    Result<std::string> result(Error("error"));
    ASSERT_FALSE(result);
    result = std::string("foo");
    ASSERT_TRUE(result);
}

TEST_F(ResultTest, ValueOr)
{
    Result<int> result(Error("error"));
    ASSERT_EQ(result.value_or(42), 42);
    result = 43;
    ASSERT_EQ(result.value_or(42), 43);
}

TEST_F(ResultTest, ArrowOperator)
{
    auto result = Result<std::string>("foo");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);
    result->append("bar");
    ASSERT_EQ(result->size(), 6);
}
