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
    void CreateTestJsonFile(const std::string& content)
    {
        std::ofstream file(m_testJsonFile);
        file << content;
        file.close();
    }

    const std::string m_testJsonFile = "/tmp/test_telemetry.json";
};

// Test singleton behavior
TEST_F(TelemetryTest, SingletonBehavior)
{
    auto& instance1 = Telemetry::TelemetryManager::GetInstance();
    auto& instance2 = Telemetry::TelemetryManager::GetInstance();

    // Both references should point to the same instance
    EXPECT_EQ(&instance1, &instance2);
}

// Test initialization with default parameters
TEST_F(TelemetryTest, InitializeDefault)
{
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();

    EXPECT_FALSE(telemetry.IsInitialized());

    bool result = telemetry.Initialize();
    EXPECT_TRUE(result);
    EXPECT_TRUE(telemetry.IsInitialized());
}

// Test initialization with custom parameters
TEST_F(TelemetryTest, InitializeWithCustomParameters)
{
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();

    bool result = telemetry.Initialize(true, 10);
    EXPECT_TRUE(result);
    EXPECT_TRUE(telemetry.IsInitialized());
}

// Test double initialization - should return true but not reinitialize
TEST_F(TelemetryTest, DoubleInitialization)
{
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();

    bool result1 = telemetry.Initialize();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(telemetry.IsInitialized());

    // Second initialization should return true but not change state
    bool result2 = telemetry.Initialize();
    EXPECT_TRUE(result2);
    EXPECT_TRUE(telemetry.IsInitialized());
}

// Test shutdown without initialization
TEST_F(TelemetryTest, ShutdownWithoutInitialization)
{
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();

    EXPECT_FALSE(telemetry.IsInitialized());

    // Should not crash
    EXPECT_NO_THROW(telemetry.Shutdown());
    EXPECT_FALSE(telemetry.IsInitialized());
}

// Test normal shutdown after initialization
TEST_F(TelemetryTest, NormalShutdown)
{
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();

    telemetry.Initialize();
    EXPECT_TRUE(telemetry.IsInitialized());

    telemetry.Shutdown();
    EXPECT_FALSE(telemetry.IsInitialized());
}
