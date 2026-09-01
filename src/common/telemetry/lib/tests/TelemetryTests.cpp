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
    std::string m_testDir;
    std::string m_testJsonFile;

    void SetUp() override
    {
        // Create a temporary directory for test files
        char tmpDir[] = "/tmp/telemetry_test_XXXXXX";
        ASSERT_NE(nullptr, mkdtemp(tmpDir));
        m_testDir = std::string(tmpDir);
        m_testJsonFile = m_testDir + "/test_telemetry.json";
    }

    void TearDown() override
    {
        // Clean up test directory
        if (!m_testDir.empty())
        {
            std::string cmd = "rm -rf " + m_testDir;
            system(cmd.c_str());
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
};

// Test processing a non-existent file
TEST_F(TelemetryTest, ProcessNonExistentFile)
{
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile("/non/existent/file.json"));
}

// Test processing an empty file
TEST_F(TelemetryTest, ProcessEmptyFile)
{
    ASSERT_TRUE(CreateTestJsonFile(""));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile)); // Empty file should be processed successfully
}

// Test processing a file with valid JSON lines
TEST_F(TelemetryTest, ProcessValidJsonFile)
{
    // Create a test file with a valid event
    std::string validJson = R"({"EventName": "TestEvent", "TestParam": "value"})";
    ASSERT_TRUE(CreateTestJsonFile(validJson));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

// Test processing a file with invalid JSON
TEST_F(TelemetryTest, ProcessInvalidJsonFile)
{
    ASSERT_TRUE(CreateTestJsonFile("invalid json content"));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

// Test processing a file with mixed valid and invalid JSON lines
TEST_F(TelemetryTest, ProcessMixedJsonFile)
{
    std::string mixedContent = R"({"EventName": "TestEvent", "TestParam": "value"}
invalid json line
{"EventName": "AnotherEvent", "Param": "value2"})";
    ASSERT_TRUE(CreateTestJsonFile(mixedContent));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

// Test processing a file with multiple valid JSON lines
TEST_F(TelemetryTest, ProcessMultipleValidJsonLines)
{
    std::string multipleLines = R"({"EventName": "Event1", "Param1": "value1"}
{"EventName": "Event2", "Param2": "value2"}
{"EventName": "Event3", "Param3": "value3"})";
    ASSERT_TRUE(CreateTestJsonFile(multipleLines));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

// Test events
TEST_F(TelemetryTest, ProcessRuleCompleteEvent)
{
    std::string realEvent = R"({"EventName":"RuleComplete","Timestamp":"2025-10-17 22:52:56+0000","ComponentName":"SecurityBaseline","ObjectName":"auditEnsureAuditdInstalled","ObjectResult":"0","Microseconds":"29","DistroName":"CentOS","CorrelationId":"","Version":"1.0.5.20251017-g03b36b7d"})";
    ASSERT_TRUE(CreateTestJsonFile(realEvent));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

TEST_F(TelemetryTest, ProcessRuleCompleteMissingComponentNameEvent)
{
    std::string realEvent = R"({"EventName":"RuleComplete","Timestamp":"2025-10-17 22:52:56+0000","ComponentName":"SecurityBaseline","ObjectResult":"0","Microseconds":"29","DistroName":"CentOS","CorrelationId":"","Version":"1.0.5.20251017-g03b36b7d"})";
    ASSERT_TRUE(CreateTestJsonFile(realEvent));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

TEST_F(TelemetryTest, ProcessBaselineRunEvent)
{
    std::string realEvent = R"({"EventName":"BaselineRun","Timestamp":"2025-10-17 22:52:56+0000","BaselineName":"Azure Security Baseline for Linux","Mode":"audit-only","DurationSeconds":"8.87","DistroName":"CentOS","CorrelationId":"","Version":"1.0.5.20251017-g03b36b7d"})";
    ASSERT_TRUE(CreateTestJsonFile(realEvent));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

TEST_F(TelemetryTest, ProcessBaselineRunMissingTimestampEvent)
{
    std::string realEvent = R"({"EventName":"BaselineRun","BaselineName":"Azure Security Baseline for Linux","Mode":"audit-only","DurationSeconds":"8.87","DistroName":"CentOS","CorrelationId":"","Version":"1.0.5.20251017-g03b36b7d"})";
    ASSERT_TRUE(CreateTestJsonFile(realEvent));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

TEST_F(TelemetryTest, ProcessStatusTraceEvent)
{
    std::string realEvent = R"({"EventName":"StatusTrace","Timestamp":"2025-10-17 22:52:56+0000","FileName":"/workspaces/azure-osconfig/src/common/asb/Asb.c","LineNumber":"1109","FunctionName":"AsbShutdown","RuleCodename":"auditEnsureSmbWithSambaIsDisabled","CallingFunctionName":"TestingStatusTrace","ResultCode":"0","ResultString":"OK","ScenarioName":"TestingScenario","Microseconds":"101807","DistroName":"CentOS","CorrelationId":"","Version":"1.0.5.20251017-g03b36b7d"})";
    ASSERT_TRUE(CreateTestJsonFile(realEvent));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_TRUE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}

TEST_F(TelemetryTest, ProcessStatusTraceMissingScenarioNameEvent)
{
    std::string realEvent = R"({"EventName":"StatusTrace","Timestamp":"2025-10-17 22:52:56+0000","FileName":"/workspaces/azure-osconfig/src/common/asb/Asb.c","LineNumber":"1109","FunctionName":"AsbShutdown","RuleCodename":"auditEnsureSmbWithSambaIsDisabled","CallingFunctionName":"TestingStatusTrace","ResultCode":"0","Microseconds":"101807","DistroName":"CentOS","CorrelationId":"","Version":"1.0.5.20251017-g03b36b7d"})";
    ASSERT_TRUE(CreateTestJsonFile(realEvent));
    Telemetry::TelemetryManager telemetryManager(false, std::chrono::seconds(1));
    EXPECT_FALSE(telemetryManager.ProcessJsonFile(m_testJsonFile));
}
