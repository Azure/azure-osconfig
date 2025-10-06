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
        const char* fileName = OSConfigTelemetryGetFileName();
        std::string filePath = (fileName != nullptr) ? std::string(fileName) : std::string();

        OSConfigTelemetryCleanup();

        if (!filePath.empty())
        {
            remove(filePath.c_str());
        }
    }
};

TEST_F(TelemetryTest, InitCreatesTelemetryFile)
{
    OSConfigTelemetryInit();

    EXPECT_NE(nullptr, OSConfigTelemetryGetFile());
    EXPECT_NE(nullptr, OSConfigTelemetryGetFileName());
    EXPECT_EQ(1, OSConfigTelemetryIsInitialized());

    std::string filePath(OSConfigTelemetryGetFileName());
    struct stat fileInfo;
    EXPECT_EQ(0, stat(filePath.c_str(), &fileInfo));
}

TEST_F(TelemetryTest, InitIsIdempotent)
{
    OSConfigTelemetryInit();
    FILE* firstHandle = OSConfigTelemetryGetFile();
    std::string firstPath(OSConfigTelemetryGetFileName());

    OSConfigTelemetryInit();

    EXPECT_EQ(firstHandle, OSConfigTelemetryGetFile());
    EXPECT_EQ(firstPath, std::string(OSConfigTelemetryGetFileName()));
    EXPECT_EQ(1, OSConfigTelemetryIsInitialized());
}

TEST_F(TelemetryTest, AppendJsonWritesSingleLine)
{
    const char* sampleJson = "{\"EventName\":\"UnitTest\"}";

    OSConfigTelemetryAppendJSON(sampleJson);

    EXPECT_NE(nullptr, OSConfigTelemetryGetFile());
    EXPECT_EQ(1, OSConfigTelemetryIsInitialized());
    ASSERT_NE(nullptr, OSConfigTelemetryGetFileName());

    std::string filePath(OSConfigTelemetryGetFileName());
    std::ifstream input(filePath);
    ASSERT_TRUE(input.is_open());

    std::string line;
    ASSERT_TRUE(static_cast<bool>(std::getline(input, line)));
    EXPECT_EQ(std::string(sampleJson), line);
}

TEST_F(TelemetryTest, CleanupResetsTelemetryState)
{
    OSConfigTelemetryInit();
    ASSERT_NE(nullptr, OSConfigTelemetryGetFileName());
    std::string filePath(OSConfigTelemetryGetFileName());

    OSConfigTelemetryCleanup();

    EXPECT_EQ(nullptr, OSConfigTelemetryGetFile());
    EXPECT_EQ(nullptr, OSConfigTelemetryGetFileName());
    EXPECT_EQ(0, OSConfigTelemetryIsInitialized());

    struct stat fileInfo;
    EXPECT_EQ(0, stat(filePath.c_str(), &fileInfo));

    EXPECT_EQ(0, remove(filePath.c_str()));
}
