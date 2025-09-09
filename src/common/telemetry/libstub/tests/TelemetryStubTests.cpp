// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Telemetry.h>

class TelemetryStubTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test C interface functions
TEST_F(TelemetryStubTest, TelemetryOpen_ReturnsNull)
{
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();
    EXPECT_EQ(NULL, handle);
}

TEST_F(TelemetryStubTest, TelemetryClose_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;
    int result = OSConfigTelemetryClose(&handle);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetryLogEvent_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;
    const char* eventName = "test_event";
    const char* keyValuePairs[] = {"key1", "value1", "key2", "value2"};
    int pairCount = 2;

    int result = OSConfigTelemetryLogEvent(handle, eventName, keyValuePairs, pairCount);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetryLogEvent_NullParams_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;

    int result = OSConfigTelemetryLogEvent(handle, NULL, NULL, 0);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetrySetBinaryDirectory_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;
    const char* directory = "/tmp/test";

    int result = OSConfigTelemetrySetBinaryDirectory(handle, directory);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetrySetBinaryDirectory_NullDirectory_ReturnsSuccess)
{
    OSConfigTelemetryHandle handle = NULL;

    int result = OSConfigTelemetrySetBinaryDirectory(handle, NULL);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, TelemetryGetFilepath_ReturnsNull)
{
    OSConfigTelemetryHandle handle = NULL;

    const char* result = OSConfigTelemetryGetFilepath(handle);
    EXPECT_EQ(NULL, result);
}
