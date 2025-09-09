// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <telemetry.h>

class TelemetryStubTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test C interface functions
TEST_F(TelemetryStubTest, TelemetryOpen_ReturnsNull)
{
    OSConfigTelemetryHandle handle = TelemetryOpen();
    EXPECT_EQ(NULL, handle);
}

TEST_F(TelemetryStubTest, TelemetryClose_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;
    int result = TelemetryClose(&handle);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetryLogEvent_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;
    const char* eventName = "test_event";
    const char* keyValuePairs[] = {"key1", "value1", "key2", "value2"};
    int pairCount = 2;

    int result = TelemetryLogEvent(handle, eventName, keyValuePairs, pairCount);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetryLogEvent_NullParams_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;

    int result = TelemetryLogEvent(handle, NULL, NULL, 0);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetrySetBinaryDirectory_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;
    const char* directory = "/tmp/test";

    int result = TelemetrySetBinaryDirectory(handle, directory);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetrySetBinaryDirectory_NullDirectory_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;

    int result = TelemetrySetBinaryDirectory(handle, NULL);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetryGetFilepath_ReturnsNull)
{
    OSConfigTelemetryHandle handle = NULL;

    const char* result = TelemetryGetFilepath(handle);
    EXPECT_EQ(NULL, result);
}
