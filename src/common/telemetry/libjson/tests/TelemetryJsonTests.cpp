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
        if (!file.is_open()) {
            return "";
        }

        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            if (!content.empty()) {
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
        if (!jsonValue) {
            return false;
        }

        JSON_Object* jsonObject = json_value_get_object(jsonValue);
        if (!jsonObject) {
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

    const char* OSConfigTelemetryGetFilepath(OSConfigTelemetryHandle handle) {
        if (handle == NULL) {
            return NULL;
        }

        struct TelemetryLogger* logger = (struct TelemetryLogger*)handle;

        if (!logger->isOpen) {
            return NULL;
        }

        return logger->filename;
    }
};

// Basic functionality tests
TEST_F(TelemetryJsonTest, OpenAndClose_Success)
{
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();

    EXPECT_NE(nullptr, handle);

    int result = OSConfigTelemetryClose(&handle);
    EXPECT_EQ(0, result);
}

TEST_F(TelemetryJsonTest, OpenMultiple_Success)
{
    OSConfigTelemetryHandle handle1 = OSConfigTelemetryOpen();
    OSConfigTelemetryHandle handle2 = OSConfigTelemetryOpen();

    EXPECT_NE(nullptr, handle1);
    EXPECT_NE(nullptr, handle2);
    EXPECT_NE(handle1, handle2);

    EXPECT_EQ(0, OSConfigTelemetryClose(&handle1));
    EXPECT_EQ(0, OSConfigTelemetryClose(&handle2));
}

TEST_F(TelemetryJsonTest, CloseNullHandle_Failure)
{
    int result = OSConfigTelemetryClose(nullptr);
    EXPECT_EQ(-1, result);
}

TEST_F(TelemetryJsonTest, CloseHandleTwice_Failure)
{
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();
    ASSERT_NE(nullptr, handle);

    int result1 = OSConfigTelemetryClose(&handle);
    EXPECT_EQ(0, result1);

    int result2 = OSConfigTelemetryClose(&handle);
    EXPECT_EQ(-1, result2);
}

// Event logging tests
TEST_F(TelemetryJsonTest, LogEvent_ValidEventWithHandle_Success)
{
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();
    ASSERT_NE(nullptr, handle);

    const char* eventName = "TestEvent";

    const char* keyValuePairs[] = {
        "key1", "value1",
        "key2", "42",
        "key3", "true"
    };
    const size_t keyCount = sizeof(keyValuePairs) / sizeof(keyValuePairs[0]) / 2;

    int result = OSConfigTelemetryLogEvent(handle, eventName, keyValuePairs, keyCount);
    EXPECT_EQ(0, result);

    std::string filePath = OSConfigTelemetryGetFilepath(handle);
    ASSERT_FALSE(filePath.empty());
    EXPECT_EQ(0, OSConfigTelemetryClose(&handle));

    // Verify file contents
    std::string fileContents = readFileContents(filePath);
    EXPECT_FALSE(fileContents.empty());

    // Validate JSON structure
    std::istringstream iss(fileContents);
    std::string line;
    while (std::getline(iss, line)) {
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
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();
    ASSERT_NE(nullptr, handle);

    const char* eventName = "SampleEvent";
    int result = OSConfigTelemetryLogEvent(handle, eventName, nullptr, 0);
    EXPECT_EQ(0, result);

    EXPECT_EQ(0, OSConfigTelemetryClose(&handle));
}

// TelemetryGetFilepath tests
TEST_F(TelemetryJsonTest, GetFilepath_ValidHandle_Success)
{
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();
    ASSERT_NE(nullptr, handle);

    const char* filepath = OSConfigTelemetryGetFilepath(handle);
    EXPECT_NE(nullptr, filepath);
    EXPECT_NE('\0', filepath[0]); // Should not be empty string

    // Filepath should start with /tmp/telemetry_ and end with .json
    std::string filepathStr(filepath);
    EXPECT_TRUE(filepathStr.find("/tmp/telemetry_") == 0);
    EXPECT_TRUE(filepathStr.find(".json") == filepathStr.length() - 5);

    EXPECT_EQ(0, OSConfigTelemetryClose(&handle));
}

TEST_F(TelemetryJsonTest, GetFilepath_NullHandle_Failure)
{
    const char* filepath = OSConfigTelemetryGetFilepath(nullptr);
    EXPECT_EQ(nullptr, filepath);
}

TEST_F(TelemetryJsonTest, GetFilepath_MultipleHandles_UniqueFilepaths)
{
    OSConfigTelemetryHandle handle1 = OSConfigTelemetryOpen();
    OSConfigTelemetryHandle handle2 = OSConfigTelemetryOpen();
    ASSERT_NE(nullptr, handle1);
    ASSERT_NE(nullptr, handle2);

    const char* filepath1 = OSConfigTelemetryGetFilepath(handle1);
    const char* filepath2 = OSConfigTelemetryGetFilepath(handle2);

    EXPECT_NE(nullptr, filepath1);
    EXPECT_NE(nullptr, filepath2);
    EXPECT_STRNE(filepath1, filepath2); // Should be different filepaths

    EXPECT_EQ(0, OSConfigTelemetryClose(&handle1));
    EXPECT_EQ(0, OSConfigTelemetryClose(&handle2));
}

TEST_F(TelemetryJsonTest, GetFilepath_AfterClose_InvalidResult)
{
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();
    ASSERT_NE(nullptr, handle);

    const char* filepath = OSConfigTelemetryGetFilepath(handle);
    EXPECT_NE(nullptr, filepath);

    // Store filepath for later verification
    std::string originalFilepath(filepath);

    EXPECT_EQ(0, OSConfigTelemetryClose(&handle));

    // After closing, getting filepath should return null or invalid result
    const char* filepathAfterClose = OSConfigTelemetryGetFilepath(handle);
    EXPECT_EQ(nullptr, filepathAfterClose);
}

TEST_F(TelemetryJsonTest, GetFilepath_FileExists_Success)
{
    OSConfigTelemetryHandle handle = OSConfigTelemetryOpen();
    ASSERT_NE(nullptr, handle);

    const char* filepath = OSConfigTelemetryGetFilepath(handle);
    EXPECT_NE(nullptr, filepath);

    // Check if file exists and is accessible
    struct stat fileStat;
    int statResult = stat(filepath, &fileStat);
    EXPECT_EQ(0, statResult);
    EXPECT_TRUE(S_ISREG(fileStat.st_mode)); // Should be a regular file

    EXPECT_EQ(0, OSConfigTelemetryClose(&handle));
}

TEST_F(TelemetryJsonTest, GetModuleDirectory_ReturnsValidPath)
{
    const char* moduleDir = OSConfigTelemetryGetModuleDirectory();

    // Should not be NULL - dladdr should find the current module
    EXPECT_NE(nullptr, moduleDir);
    if (moduleDir) {
        EXPECT_NE('\0', moduleDir[0]);

        // Should be an absolute path (start with '/') or relative path (start with '.')
        EXPECT_TRUE(moduleDir[0] == '/' || moduleDir[0] == '.');

        // Should not end with '/' (directory names typically don't)
        size_t len = strlen(moduleDir);
        if (len > 0) {
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
