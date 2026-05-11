// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <FileExists.h>
#include <MockContext.h>
#include <fstream>
#include <gtest/gtest.h>

using ComplianceEngine::AuditFileExists;
using ComplianceEngine::Error;
using ComplianceEngine::FileExistsParams;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureFileExistsTest : public ::testing::Test
{
protected:
    char mFilename[PATH_MAX] = "/tmp/EnsureFileExistsTest.XXXXXX";
    MockContext mContext;
    IndicatorsTree mIndicators;

    void SetUp() override
    {
        mIndicators.Push("EnsureFileExistsTest");
        ASSERT_NE(-1, mkstemp(mFilename));
    }

    void TearDown() override
    {
        ASSERT_EQ(unlink(mFilename), 0);
    }
};

TEST_F(EnsureFileExistsTest, Exists)
{
    std::ofstream file(mFilename);
    FileExistsParams params;
    params.filename = mFilename;
    auto result = AuditFileExists(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFileExistsTest, DoesNotExist)
{
    FileExistsParams params;
    params.filename = mFilename;
    auto result = AuditFileExists(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
