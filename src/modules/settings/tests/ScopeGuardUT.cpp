// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include "gtest/gtest.h"
#include "ScopeGuard.h"

TEST(ScopeGuard, General)
{
    int test = 0;
    ScopeGuard sg{[&]()
    {
        EXPECT_EQ(1, test);
    }};
    test++;
}

TEST(ScopeGuard, Dismiss)
{
    std::string message = "The text should not appear in output as scope guard is dismissed.\n";
    ScopeGuard sg{[&]()
    {
        std::cout << message;
    }};
    sg.Dismiss();
    EXPECT_EQ(0, 0);
}

TEST(ScopeGuard, MultipleScopes)
{
    int test = 0;
    ScopeGuard outer_sg{[&]()
    {
        test++;
        EXPECT_EQ(test, 2);
    }};
    {
        ScopeGuard inner_sg{[&]()
        {
            test++;
        }};
    }
    EXPECT_EQ(test, 1);
}

TEST(Scope, DismissMultipleScopes)
{
    int test = 0;
    {
        ScopeGuard inner_sg {[&]()
        {
            test++;
        }};
        inner_sg.Dismiss();
    }
    EXPECT_EQ(test, 0);
}