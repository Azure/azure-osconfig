// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <MockContext.h>
#include <UsersIterator.h>
#include <gtest/gtest.h>

using ComplianceEngine::Error;
using ComplianceEngine::Result;
using ComplianceEngine::UsersRange;

class UsersIteratorTest : public ::testing::Test
{
protected:
    MockContext mContext;
};

TEST_F(UsersIteratorTest, NonExistentFile)
{
    auto result = UsersRange::Make("/tmp/somenoneexistentfilename", mContext.GetLogHandle());
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, ENOENT);
    ASSERT_EQ(result.Error().message, "Failed to create UsersRange: No such file or directory");
}
