// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

#include <Telemetry.hpp>

class TelemetryTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        // Clean up any test files
        if (std::remove(m_testJsonFile.c_str()) == 0)
        {
            // File was successfully removed
        }
    }

    // Helper function to create a test JSON file
    bool CreateTestJsonFile(const std::string& content)
    {
        std::ofstream file(m_testJsonFile);
        if (!file.is_open())
        {
            return false;
        }
        file << content;
        file.close();
        return true;
    }

    const std::string m_testJsonFile = "/tmp/test_telemetry.json";
};

// Test processing a non-existent file
TEST_F(TelemetryTest, ProcessNonExistentFile)
{
    Telemetry::TelemetryManager telemetryManager(false, 1);
    EXPECT_FALSE(telemetryManager.ProcessJsonFile("/non/existent/file.json"));
}

// Test processing an empty file
TEST_F(TelemetryTest, ProcessEmptyFile)
{
    ASSERT_TRUE(CreateTestJsonFile(""));
    Telemetry::TelemetryManager telemetryManager(false, 1);
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile)); // Empty file should be processed successfully
}

// Test processing a file with valid JSON lines
TEST_F(TelemetryTest, ProcessValidJsonFile)
{
    // Create a test file with a valid event
    std::string validJson = R"({"EventName": "TestEvent", "TestParam": "value"})";
    ASSERT_TRUE(CreateTestJsonFile(validJson));
    Telemetry::TelemetryManager telemetryManager(false, 1);
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

// Test processing a file with invalid JSON
TEST_F(TelemetryTest, ProcessInvalidJsonFile)
{
    ASSERT_TRUE(CreateTestJsonFile("invalid json content"));
    Telemetry::TelemetryManager telemetryManager(false, 1);
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile)); // Method should return true even if individual lines fail
}

// Test processing a file with mixed valid and invalid JSON lines
TEST_F(TelemetryTest, ProcessMixedJsonFile)
{
    std::string mixedContent = R"({"EventName": "TestEvent", "TestParam": "value"}
invalid json line
{"EventName": "AnotherEvent", "Param": "value2"})";
    ASSERT_TRUE(CreateTestJsonFile(mixedContent));
    Telemetry::TelemetryManager telemetryManager(false, 1);
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

// Test processing a file with multiple valid JSON lines
TEST_F(TelemetryTest, ProcessMultipleValidJsonLines)
{
    std::string multipleLines = R"({"EventName": "Event1", "Param1": "value1"}
{"EventName": "Event2", "Param2": "value2"}
{"EventName": "Event3", "Param3": "value3"})";
    ASSERT_TRUE(CreateTestJsonFile(multipleLines));
    Telemetry::TelemetryManager telemetryManager(false, 1);
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}
