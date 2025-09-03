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
TEST_F(TelemetryStubTest, Telemetry_Open_ReturnsNull)
{
    TelemetryHandle handle = Telemetry_Open();
    EXPECT_EQ(NULL, handle);
}

TEST_F(TelemetryStubTest, Telemetry_Close_ReturnsSuccess)
{
    TelemetryHandle handle = NULL;
    int result = Telemetry_Close(&handle);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, Telemetry_LogEvent_ReturnsSuccess)
{
    TelemetryHandle handle = NULL;
    const char* eventName = "test_event";
    const char* keyValuePairs[] = {"key1", "value1", "key2", "value2"};
    int pairCount = 2;

    int result = Telemetry_LogEvent(handle, eventName, keyValuePairs, pairCount);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, Telemetry_LogEvent_NullParams_ReturnsSuccess)
{
    TelemetryHandle handle = NULL;

    int result = Telemetry_LogEvent(handle, NULL, NULL, 0);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, Telemetry_SetBinaryDirectory_ReturnsSuccess)
{
    TelemetryHandle handle = NULL;
    const char* directory = "/tmp/test";

    int result = Telemetry_SetBinaryDirectory(handle, directory);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, Telemetry_SetBinaryDirectory_NullDirectory_ReturnsSuccess)
{
    TelemetryHandle handle = NULL;

    int result = Telemetry_SetBinaryDirectory(handle, NULL);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryStubTest, Telemetry_GetFilepath_ReturnsNull)
{
    TelemetryHandle handle = NULL;

    const char* result = Telemetry_GetFilepath(handle);
    EXPECT_EQ(NULL, result);
}
