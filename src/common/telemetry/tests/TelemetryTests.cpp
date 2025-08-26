// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <chrono>
#include "telemetry.h"

class TelemetryTest : public ::testing::Test
{
protected:
    // void SetUp() override
    // {
    //     // Ensure clean state before each test
    //     auto& telemetry = Telemetry::TelemetryManager::GetInstance();
    //     if (telemetry.IsInitialized())
    //     {
    //         telemetry.Shutdown();
    //     }
    // }

    void TearDown() override
    {
        // Clean up after each test
        // auto& telemetry = Telemetry::TelemetryManager::GetInstance();
        // if (telemetry.IsInitialized())
        // {
        //     telemetry.Shutdown();
        // }

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

// Test LogEvent without initialization
// TEST_F(TelemetryTest, LogEventWithoutInitialization)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();

//     EXPECT_FALSE(telemetry.IsInitialized());

//     // Should not crash even if not initialized
//     EXPECT_NO_THROW(telemetry.LogEvent("TestEvent"));
// }

// // Test LogEvent with initialization
// TEST_F(TelemetryTest, LogEventWithInitialization)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();

//     telemetry.Initialize();
//     EXPECT_TRUE(telemetry.IsInitialized());

//     EXPECT_NO_THROW(telemetry.LogEvent("TestEvent"));
//     EXPECT_NO_THROW(telemetry.LogEvent("AnotherEvent"));
//     EXPECT_NO_THROW(telemetry.LogEvent(""));  // Empty event name
// }

// // Test ProcessJsonFile with non-existent file
// TEST_F(TelemetryTest, ProcessJsonFileNonExistent)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();
//     telemetry.Initialize();

//     bool result = telemetry.ProcessJsonFile("/non/existent/file.json");
//     EXPECT_FALSE(result);
// }

// // Test ProcessJsonFile with empty file
// TEST_F(TelemetryTest, ProcessJsonFileEmpty)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();
//     telemetry.Initialize();

//     CreateTestJsonFile("");

//     bool result = telemetry.ProcessJsonFile(m_testJsonFile);
//     EXPECT_TRUE(result);  // Empty file should be processed successfully
// }

// // Test ProcessJsonFile with valid JSON lines
// TEST_F(TelemetryTest, ProcessJsonFileValidJson)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();
//     telemetry.Initialize();

//     std::string jsonContent =
//         R"({"event": "startup", "timestamp": "2025-01-01T00:00:00Z"})" "\n"
//         R"({"event": "shutdown", "timestamp": "2025-01-01T01:00:00Z"})" "\n"
//         R"({"event": "error", "message": "Test error", "code": 123})" "\n";

//     CreateTestJsonFile(jsonContent);

//     bool result = telemetry.ProcessJsonFile(m_testJsonFile);
//     EXPECT_TRUE(result);
// }

// // Test ProcessJsonFile with invalid JSON lines
// TEST_F(TelemetryTest, ProcessJsonFileInvalidJson)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();
//     telemetry.Initialize();

//     std::string jsonContent =
//         R"({"event": "startup", "timestamp": "2025-01-01T00:00:00Z"})" "\n"
//         "invalid json line\n"
//         R"({"event": "shutdown"})" "\n";

//     CreateTestJsonFile(jsonContent);

//     // Should still process valid lines and not crash on invalid ones
//     bool result = telemetry.ProcessJsonFile(m_testJsonFile);
//     EXPECT_TRUE(result);
// }

// // Test ProcessJsonFile without initialization
// TEST_F(TelemetryTest, ProcessJsonFileWithoutInitialization)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();

//     CreateTestJsonFile(R"({"event": "test"})");

//     bool result = telemetry.ProcessJsonFile(m_testJsonFile);
//     EXPECT_FALSE(result);
// }

// // Test thread safety of singleton access
// TEST_F(TelemetryTest, ThreadSafetySingleton)
// {
//     const int numThreads = 10;
//     std::vector<std::thread> threads;
//     std::vector<Telemetry::TelemetryManager*> instances(numThreads);

//     for (int i = 0; i < numThreads; ++i)
//     {
//         threads.emplace_back([i, &instances]()
//         {
//             instances[i] = &Telemetry::TelemetryManager::GetInstance();
//         });
//     }

//     for (auto& t : threads)
//     {
//         t.join();
//     }

//     // All instances should be the same
//     for (int i = 1; i < numThreads; ++i)
//     {
//         EXPECT_EQ(instances[0], instances[i]);
//     }
// }

// // Test concurrent initialization and shutdown
// TEST_F(TelemetryTest, ConcurrentInitializeShutdown)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();

//     const int numThreads = 5;
//     std::vector<std::thread> threads;

//     for (int i = 0; i < numThreads; ++i)
//     {
//         threads.emplace_back([&telemetry, i]()
//         {
//             if (i % 2 == 0)
//             {
//                 telemetry.Initialize();
//                 std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                 telemetry.LogEvent("ConcurrentTest");
//             }
//             else
//             {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(5));
//                 telemetry.LogEvent("DelayedTest");
//             }
//         });
//     }

//     for (auto& t : threads)
//     {
//         t.join();
//     }

//     // Should not crash and telemetry should be in a valid state
//     if (telemetry.IsInitialized())
//     {
//         telemetry.Shutdown();
//     }
// }

// // Test initialization with edge case teardown times
// TEST_F(TelemetryTest, InitializeEdgeCaseTeardownTimes)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();

//     // Test with minimum teardown time
//     EXPECT_TRUE(telemetry.Initialize(false, 0));
//     EXPECT_TRUE(telemetry.IsInitialized());
//     telemetry.Shutdown();

//     // Test with negative teardown time (should use default)
//     EXPECT_TRUE(telemetry.Initialize(false, -1));
//     EXPECT_TRUE(telemetry.IsInitialized());
//     telemetry.Shutdown();

//     // Test with large teardown time
//     EXPECT_TRUE(telemetry.Initialize(false, 3600));
//     EXPECT_TRUE(telemetry.IsInitialized());
// }

// // Test state consistency after multiple operations
// TEST_F(TelemetryTest, StateConsistency)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();

//     // Initial state
//     EXPECT_FALSE(telemetry.IsInitialized());

//     // Initialize
//     telemetry.Initialize();
//     EXPECT_TRUE(telemetry.IsInitialized());

//     // Log events
//     telemetry.LogEvent("Event1");
//     telemetry.LogEvent("Event2");
//     EXPECT_TRUE(telemetry.IsInitialized());

//     // Process JSON
//     CreateTestJsonFile(R"({"event": "test"})");
//     telemetry.ProcessJsonFile(m_testJsonFile);
//     EXPECT_TRUE(telemetry.IsInitialized());

//     // Shutdown
//     telemetry.Shutdown();
//     EXPECT_FALSE(telemetry.IsInitialized());
// }

// // Test with special characters in event names
// TEST_F(TelemetryTest, LogEventSpecialCharacters)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();
//     telemetry.Initialize();

//     EXPECT_NO_THROW(telemetry.LogEvent("Event with spaces"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event_with_underscores"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event-with-dashes"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event.with.dots"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event/with/slashes"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event\\with\\backslashes"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event\"with\"quotes"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event'with'apostrophes"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event\nwith\nnewlines"));
//     EXPECT_NO_THROW(telemetry.LogEvent("Event\twith\ttabs"));
// }

// // Test large JSON file processing
// TEST_F(TelemetryTest, ProcessLargeJsonFile)
// {
//     auto& telemetry = Telemetry::TelemetryManager::GetInstance();
//     telemetry.Initialize();

//     std::ostringstream largeContent;
//     const int numLines = 1000;

//     for (int i = 0; i < numLines; ++i)
//     {
//         largeContent << R"({"event": "event_)" << i << R"(", "id": )" << i << "}\n";
//     }

//     CreateTestJsonFile(largeContent.str());

//     bool result = telemetry.ProcessJsonFile(m_testJsonFile);
//     EXPECT_TRUE(result);
// }
