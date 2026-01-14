// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <EnsureFileExists.h>
#include <MockContext.h>
#include <fstream>
#include <gtest/gtest.h>

using ComplianceEngine::AuditEnsureFileExists;
using ComplianceEngine::AuditEnsureFileExistsParams;
using ComplianceEngine::Error;
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
    AuditEnsureFileExistsParams params;
    params.filename = mFilename;
    auto result = AuditEnsureFileExists(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFileExistsTest, DoesNotExist)
{
    AuditEnsureFileExistsParams params;
    params.filename = mFilename;
    auto result = AuditEnsureFileExists(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
