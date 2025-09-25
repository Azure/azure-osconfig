// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <parson.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <TelemetryJson.h>

#if BUILD_TELEMETRY
class TelemetryJsonTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        // Clean up test files after each test
        cleanupTestFiles();
    }

private:
    void cleanupTestFiles()
    {
        // Remove any telemetry json files that might have been created during tests
        system("rm -f /tmp/telemetry_*.json");
    }

protected:
    // Helper function to read file contents
    std::string readFileContents(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return "";
        }

        std::string content;
        std::string line;
        while (std::getline(file, line))
        {
            if (!content.empty())
            {
                content += "\n";
            }
            content += line;
        }

        return content;
    }

    // Helper function to parse JSON and validate structure
    bool validateJsonLine(const std::string& jsonLine, const std::string& expectedEventName)
    {
        JSON_Value* jsonValue = json_parse_string(jsonLine.c_str());
        if (!jsonValue)
        {
            return false;
        }

        JSON_Object* jsonObject = json_value_get_object(jsonValue);
        if (!jsonObject)
        {
            json_value_free(jsonValue);
            return false;
        }

        // Check if required fields exist
        const char* timestamp = json_object_get_string(jsonObject, "Timestamp");
        const char* eventName = json_object_get_string(jsonObject, "EventName");

        bool isValid = (timestamp != nullptr) &&
                      (eventName != nullptr) &&
                      (strcmp(eventName, expectedEventName.c_str()) == 0);

        json_value_free(jsonValue);
        return isValid;
    }

    // Helper function to get the current telemetry log file path
    // Since we now use a static instance, we need to access it differently
    std::string getCurrentTelemetryFilePath()
    {
        // We'll need to check if a file was created in /tmp
        system("ls /tmp/telemetry_*.json > /tmp/telemetry_files.txt 2>/dev/null");
        std::ifstream fileList("/tmp/telemetry_files.txt");
        std::string filePath;
        if (std::getline(fileList, filePath))
        {
            return filePath;
        }
        return "";
    }
};

// Basic functionality tests
TEST_F(TelemetryJsonTest, OpenAndClose_Success)
{
    int openResult = OSConfigTelemetryOpen(NULL);
    EXPECT_EQ(0, openResult);

    int closeResult = OSConfigTelemetryClose();
    EXPECT_EQ(0, closeResult);
}

TEST_F(TelemetryJsonTest, OpenMultiple_Success)
{
    // With static instance, multiple opens should succeed but use same instance
    int result1 = OSConfigTelemetryOpen(NULL);
    EXPECT_EQ(0, result1);

    int result2 = OSConfigTelemetryOpen(NULL); // Should return success since already open
    EXPECT_EQ(0, result2);

    int closeResult = OSConfigTelemetryClose();
    EXPECT_EQ(0, closeResult);
}

TEST_F(TelemetryJsonTest, CloseWithoutOpen_Failure)
{
    // Trying to close without opening should fail
    int result = OSConfigTelemetryClose();
    EXPECT_EQ(-1, result);
}

TEST_F(TelemetryJsonTest, CloseTwice_Failure)
{
    int openResult = OSConfigTelemetryOpen(NULL);
    ASSERT_EQ(0, openResult);

    int result1 = OSConfigTelemetryClose();
    EXPECT_EQ(0, result1);

    int result2 = OSConfigTelemetryClose();
    EXPECT_EQ(-1, result2);
}

// Event logging tests
TEST_F(TelemetryJsonTest, LogEvent_ValidEventWithHandle_Success)
{
    int openResult = OSConfigTelemetryOpen(NULL);
    ASSERT_EQ(0, openResult);

    const char* eventName = "TestEvent";

    const char* keyValuePairs[] = {
        "key1", "value1",
        "key2", "42",
        "key3", "true"
    };
    const size_t keyCount = sizeof(keyValuePairs) / sizeof(keyValuePairs[0]) / 2;

    int result = OSConfigTelemetryLogEvent(eventName, keyValuePairs, keyCount);
    EXPECT_EQ(0, result);

    EXPECT_EQ(0, OSConfigTelemetryClose());

    std::string filePath = getCurrentTelemetryFilePath();
    ASSERT_FALSE(filePath.empty());

    // Verify file contents
    std::string fileContents = readFileContents(filePath);
    EXPECT_FALSE(fileContents.empty());

    // Validate JSON structure
    std::istringstream iss(fileContents);
    std::string line;
    while (std::getline(iss, line))
    {
        EXPECT_TRUE(validateJsonLine(line, eventName));

        // Parse the JSON line to validate data types
        JSON_Value* jsonValue = json_parse_string(line.c_str());
        ASSERT_NE(nullptr, jsonValue);

        JSON_Object* jsonObject = json_value_get_object(jsonValue);
        ASSERT_NE(nullptr, jsonObject);

        // Ensure key1 is string
        JSON_Value* key1Value = json_object_get_value(jsonObject, "key1");
        ASSERT_NE(nullptr, key1Value);
        EXPECT_EQ(JSONString, json_value_get_type(key1Value));
        const char* key1String = json_object_get_string(jsonObject, "key1");
        EXPECT_STREQ("value1", key1String);

        // Ensure key2 is number
        JSON_Value* key2Value = json_object_get_value(jsonObject, "key2");
        ASSERT_NE(nullptr, key2Value);
        EXPECT_EQ(JSONNumber, json_value_get_type(key2Value));
        double key2Number = json_object_get_number(jsonObject, "key2");
        EXPECT_EQ(42.0, key2Number);

        // Ensure key3 is boolean
        JSON_Value* key3Value = json_object_get_value(jsonObject, "key3");
        ASSERT_NE(nullptr, key3Value);
        EXPECT_EQ(JSONBoolean, json_value_get_type(key3Value));
        int key3Boolean = json_object_get_boolean(jsonObject, "key3");
        EXPECT_EQ(1, key3Boolean); // true is represented as 1

        json_value_free(jsonValue);
    }
}

TEST_F(TelemetryJsonTest, LogEvent_Sample_Success)
{
    int openResult = OSConfigTelemetryOpen(NULL);
    ASSERT_EQ(0, openResult);

    const char* eventName = "SampleEvent";
    int result = OSConfigTelemetryLogEvent(eventName, nullptr, 0);
    EXPECT_EQ(0, result);

    EXPECT_EQ(0, OSConfigTelemetryClose());
}

// File creation and logging behavior tests
TEST_F(TelemetryJsonTest, TelemetryFileCreation_Success)
{
    int openResult = OSConfigTelemetryOpen(NULL);
    ASSERT_EQ(0, openResult);

    // Log an event to ensure file is created
    const char* eventName = "TestFileCreation";
    int logResult = OSConfigTelemetryLogEvent(eventName, nullptr, 0);
    EXPECT_EQ(0, logResult);

    EXPECT_EQ(0, OSConfigTelemetryClose());

    // Verify that a telemetry file was created
    std::string filePath = getCurrentTelemetryFilePath();
    EXPECT_FALSE(filePath.empty());

    if (!filePath.empty())
    {
        // Filepath should start with /tmp/telemetry_ and end with .json
        EXPECT_TRUE(filePath.find("/tmp/telemetry_") == 0);
        EXPECT_TRUE(filePath.find(".json") == filePath.length() - 5);

        // Check if file exists and is accessible
        struct stat fileStat;
        int statResult = stat(filePath.c_str(), &fileStat);
        EXPECT_EQ(0, statResult);
        EXPECT_TRUE(S_ISREG(fileStat.st_mode)); // Should be a regular file

        // Verify file contents
        std::string fileContents = readFileContents(filePath);
        EXPECT_FALSE(fileContents.empty());
        EXPECT_TRUE(validateJsonLine(fileContents, eventName));
    }
}

TEST_F(TelemetryJsonTest, StaticInstance_Behavior)
{
    // Test that multiple opens use the same static instance
    int openResult1 = OSConfigTelemetryOpen(NULL);
    ASSERT_EQ(0, openResult1);

    // Log an event
    const char* eventName = "StaticInstanceTest";
    int logResult1 = OSConfigTelemetryLogEvent(eventName, nullptr, 0);
    EXPECT_EQ(0, logResult1);

    // Open again (should use same instance)
    int openResult2 = OSConfigTelemetryOpen(NULL);
    EXPECT_EQ(0, openResult2);

    // Log another event
    int logResult2 = OSConfigTelemetryLogEvent(eventName, nullptr, 0);
    EXPECT_EQ(0, logResult2);

    EXPECT_EQ(0, OSConfigTelemetryClose());

    // Verify file contains both events
    std::string filePath = getCurrentTelemetryFilePath();
    EXPECT_FALSE(filePath.empty());

    if (!filePath.empty())
    {
        std::string fileContents = readFileContents(filePath);
        EXPECT_FALSE(fileContents.empty());

        // Count number of lines (should be 2)
        int lineCount = 0;
        std::istringstream iss(fileContents);
        std::string line;
        while (std::getline(iss, line))
        {
            if (!line.empty())
            {
                lineCount++;
                EXPECT_TRUE(validateJsonLine(line, eventName));
            }
        }
        EXPECT_EQ(2, lineCount);
    }
}

TEST_F(TelemetryJsonTest, GetModuleDirectory_ReturnsValidPath)
{
    const char* moduleDir = OSConfigTelemetryGetModuleDirectory();

    // Should not be NULL - dladdr should find the current module
    EXPECT_NE(nullptr, moduleDir);
    if (moduleDir)
    {
        EXPECT_NE('\0', moduleDir[0]);

        // Should be an absolute path (start with '/') or relative path (start with '.')
        EXPECT_TRUE(moduleDir[0] == '/' || moduleDir[0] == '.');

        // Should not end with '/' (directory names typically don't)
        size_t len = strlen(moduleDir);
        if (len > 0)
        {
            EXPECT_NE('/', moduleDir[len - 1]);
        }

        // The directory should exist
        struct stat dirStat;
        int statResult = stat(moduleDir, &dirStat);
        EXPECT_EQ(0, statResult);
        EXPECT_TRUE(S_ISDIR(dirStat.st_mode));
    }
}

#endif // BUILD_TELEMETRY
