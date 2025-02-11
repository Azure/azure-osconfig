// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <CommonUtils.h>
#include "ComplianceInterface.h"

using namespace std;

class ComplianceTest : public ::testing::Test
{
    protected:
        void SetUp()
        {
            ComplianceInitialize(nullptr);
        }

        void TearDown()
        {
            ComplianceShutdown();
        }
};

TEST_F(ComplianceTest, ComplianceMmiOpen)
{
    auto handle = ComplianceMmiOpen("test", 100);
    ASSERT_EQ(handle, nullptr);
}
