// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <DeliveryOptimization.h>
#include <CommonUtils.h>

using namespace std;

class DeliveryOptimizationTest : public ::testing::Test
{
    protected:
        const char* m_clientName = "Test";
        int m_normalMaxPayloadSizeBytes = 1024;

        void SetUp()
        {
            DeliveryOptimizationInitialize();
        }

        void TearDown()
        {
            DeliveryOptimizationShutdown();
        }
};

TEST_F(DeliveryOptimizationTest, MmiOpen)
{
    MMI_HANDLE handle = nullptr;
    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    DeliveryOptimizationMmiClose(handle);
}

// TODO: ...