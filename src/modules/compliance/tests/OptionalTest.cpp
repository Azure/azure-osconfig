// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include "Optional.h"

using compliance::Optional;

class OptionalTest : public ::testing::Test
{
};

TEST_F(OptionalTest, DefaultContructor)
{
    Optional<int> opt;
    ASSERT_FALSE(opt.has_value());
}

TEST_F(OptionalTest, ValueContructor)
{
    Optional<int> opt(42);
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt.value(), 42);

    opt = Optional<int>{};
    ASSERT_FALSE(opt.has_value());
}

TEST_F(OptionalTest, CopyContructor)
{
    Optional<int> opt1(42);
    Optional<int> opt2(opt1);
    ASSERT_TRUE(opt1.has_value());
    ASSERT_TRUE(opt2.has_value());
    ASSERT_EQ(opt2.value(), 42);
}

TEST_F(OptionalTest, MoveContructor)
{
    Optional<int> opt1(42);
    Optional<int> opt2(std::move(opt1));
    ASSERT_FALSE(opt1.has_value());
    ASSERT_TRUE(opt2.has_value());
    ASSERT_EQ(opt2.value(), 42);
}

TEST_F(OptionalTest, CopyAssignment)
{
    Optional<int> opt1(42);
    Optional<int> opt2;
    opt2 = opt1;
    ASSERT_TRUE(opt1.has_value());
    ASSERT_TRUE(opt2.has_value());
    ASSERT_EQ(opt2.value(), 42);
}

TEST_F(OptionalTest, MoveAssignment)
{
    Optional<int> opt1(42);
    Optional<int> opt2;
    opt2 = std::move(opt1);
    ASSERT_FALSE(opt1.has_value());
    ASSERT_TRUE(opt2.has_value());
    ASSERT_EQ(opt2.value(), 42);
}

TEST_F(OptionalTest, ValueAssignment)
{
    Optional<int> opt;
    opt = 42;
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt.value(), 42);
}

TEST_F(OptionalTest, ReferenceReturned)
{
    Optional<int> opt(42);
    opt.value() = 43;
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt.value(), 43);
}

TEST_F(OptionalTest, BoolConversion)
{
    Optional<std::string> opt;
    ASSERT_FALSE(opt);
    opt = "foo";
    ASSERT_TRUE(opt);
}

TEST_F(OptionalTest, ValueOr)
{
    Optional<int> opt;
    ASSERT_EQ(opt.value_or(42), 42);
    opt = 43;
    ASSERT_EQ(opt.value_or(42), 43);
}

TEST_F(OptionalTest, Reset)
{
    Optional<int> opt(42);
    ASSERT_TRUE(opt.has_value());
    opt.reset();
    ASSERT_FALSE(opt.has_value());
}

TEST_F(OptionalTest, ArrowOperator)
{
    auto opt = Optional<std::string>("foo");
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt->size(), 3);
    opt->append("bar");
    ASSERT_EQ(opt->size(), 6);
}
