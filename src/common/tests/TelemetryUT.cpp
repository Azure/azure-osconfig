// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <Telemetry.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>

class TelemetryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        CleanupState();
    }

    void TearDown() override
    {
        CleanupState();
    }

    void CleanupState()
    {
        OSConfigTelemetryCleanup();
        remove(TELEMETRY_TMP_FILE_NAME);
    }
};

TEST_F(TelemetryTest, InitCreatesTelemetryFile)
{
    OSConfigTelemetryInit();

    struct stat fileInfo;
    EXPECT_EQ(0, stat(TELEMETRY_TMP_FILE_NAME, &fileInfo));
}

TEST_F(TelemetryTest, AppendJsonWritesSingleLine)
{
    const char* sampleJson = "{\"EventName\":\"UnitTest\"}";

    OSConfigTelemetryAppendJSON(sampleJson);

    std::ifstream input(TELEMETRY_TMP_FILE_NAME);
    ASSERT_TRUE(input.is_open());

    std::string line;
    ASSERT_TRUE(static_cast<bool>(std::getline(input, line)));
    EXPECT_EQ(std::string(sampleJson), line);
}

TEST_F(TelemetryTest, CleanupResetsTelemetryState)
{
    OSConfigTelemetryInit();
    OSConfigTelemetryCleanup();

    struct stat fileInfo;
    EXPECT_EQ(0, stat(TELEMETRY_TMP_FILE_NAME, &fileInfo));
    EXPECT_EQ(0, remove(TELEMETRY_TMP_FILE_NAME));
}
