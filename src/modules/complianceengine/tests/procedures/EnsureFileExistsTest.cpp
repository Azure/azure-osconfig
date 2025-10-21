// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <EnsureFileExists.h>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <regex>
#include <string>
#include <unistd.h>

using ComplianceEngine::AuditEnsureFileExists;
using ComplianceEngine::AuditEnsureFileExistsParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureFileExistsTest : public ::testing::Test
{
protected:
    char mFilename[PATH_MAX] = "/tmp/permTest.XXXXXX";
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;

    void SetUp() override
    {
        mIndicators.Push("UfwStatus");
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
