// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <iostream>
#include "gtest/gtest.h"
// #include "../configfileutils/BaseUtils.h"
// #include "../configfileutils/ConfigFileUtils.h"
#include "BaseUtils.h"
#include "ConfigFileUtils.h"

using namespace std;

class ConfigFileUtilsTest : public ::testing::Test
{
    protected:
        CONFIG_FILE_HANDLE m_config = nullptr;
        const char* m_jsonPath = "test.json";
        const char* m_tomlPath = "test.toml";
        const char* m_jsonData = "{\"testNameString\": \"testValueString\", \"testNameInteger\": 123}";
        const char* m_jsonDataNested = "{\"nestedNameString\": {\"testNameString\": \"testValueString\"}, \"nestedNameInteger\" :{\"testNameInteger\": 123}}";
        const char* m_tomlData = "testNameString = \"testValueString\"";
        const char* m_testNameString = "testNameString";
        const char* m_testNameStringJson = "/testNameString";
        const char* m_nestedTestNameStringJson = "/nestedNameString/testNameString";
        const char* m_testValueString = "testValueString";
        const char* m_testNameInteger = "testNameInteger";
        const char* m_testNameIntegerJson = "/testNameInteger";
        const char* m_nestedTestNameIntegerJson = "/nestedNameInteger/testNameInteger";
        int m_testValueInteger = 123;
        const char* m_newNameString = "newNameString";
        const char* m_newNameStringJson = "/newNameString";
        const char* m_newValueString = "newValueString";
        const char* m_newNameInteger = "newNameInteger";
        const char* m_newNameIntegerJson = "/newNameInteger";
        int m_newValueInteger = 456;

        CONFIG_FILE_HANDLE CreateTestHandleAndData(const char* path, const char* data, ConfigFileFormat format)
        {
            if (nullptr != path)
            {
                ofstream ofs(path);
                if (nullptr != data)
                {
                    ofs << data;
                }
                ofs.close();

                return OpenConfigFile(path, format);
            }
            else
            {
                printf("ConfigFileUtilsTest::CreateTestHandleAndData: Invalid argument\n");
            }

            return nullptr;
        }

        bool CleanupTestHandleAndData(CONFIG_FILE_HANDLE config, const char* path)
        {
            CloseConfigFile(config);

            if (nullptr != path)
            {
                if (!(remove(path) == 0))
                {
                    printf("ConfigFileUtilsTest::CleanupTestHandleAndData: Unable to delete %s\n", path);
                    return false;
                }
            }

            return true;
        }
};

TEST_F(ConfigFileUtilsTest, Exceptions)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, m_tomlData, Testing));
    EXPECT_EQ(nullptr, ReadConfigString(m_config, "testingexceptions"));
    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(m_config, "testingexceptions"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, "testingexceptions", "testingexceptions"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigInteger(m_config, "testingexceptions", 10));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, NameNotFound)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, "invalidName"));
    // If name not found, add name with value
    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigString(m_config, m_newNameStringJson, m_newValueString));
    EXPECT_STREQ(m_newValueString, ReadConfigString(m_config, m_newNameStringJson));
    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(m_config, "invalidName"));
    // If name not found, add name with value
    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigInteger(m_config, m_newNameIntegerJson, m_newValueInteger));
    EXPECT_EQ(m_newValueInteger, ReadConfigInteger(m_config, m_newNameIntegerJson));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, m_tomlData, ConfigFileFormatToml));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, "invalidName"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, "invalidName", "replacementValue"));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, InvalidFormat)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, "This is invalid JSON", ConfigFileFormatJson));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, m_testNameString));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, "replacementValue"));
    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(m_config, m_testNameInteger));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigInteger(m_config, m_testNameInteger, 456));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, "This is invalid TOML", ConfigFileFormatToml));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, m_testNameString));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, "replacementValue"));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, FileEmpty)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, nullptr, ConfigFileFormatJson));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, m_testNameString));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, m_testValueString));
    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(m_config, m_testNameInteger));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigInteger(m_config, m_testNameInteger, 456));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, nullptr, ConfigFileFormatToml));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, m_testNameString));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, m_testValueString));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, FileNotFound)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, nullptr, ConfigFileFormatJson));
    EXPECT_EQ(0, remove(m_jsonPath));

    EXPECT_EQ(nullptr, OpenConfigFile(m_jsonPath, ConfigFileFormatJson));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, m_testNameString));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, m_testValueString));
    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(m_config, m_testNameInteger));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigInteger(m_config, m_testNameInteger, 456));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, nullptr));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, nullptr, ConfigFileFormatToml));
    EXPECT_EQ(0, remove(m_tomlPath));

    EXPECT_EQ(nullptr, OpenConfigFile(m_tomlPath, ConfigFileFormatToml));
    EXPECT_STREQ(nullptr, ReadConfigString(m_config, m_testNameString));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, m_testValueString));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, nullptr));
}

TEST_F(ConfigFileUtilsTest, NullArguemnt)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_EQ(nullptr, OpenConfigFile(nullptr, ConfigFileFormatJson));

    EXPECT_STREQ(nullptr, ReadConfigString(m_config, nullptr));
    EXPECT_STREQ(nullptr, ReadConfigString(nullptr, m_testNameString));
    EXPECT_STREQ(nullptr, ReadConfigString(nullptr, nullptr));

    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, nullptr));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, nullptr, "replacementValue"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, nullptr, nullptr));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, m_testNameString, "replacementValue"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, m_testNameString, nullptr));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, nullptr, "replacemetValue"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, nullptr, nullptr));

    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(m_config, nullptr));
    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(nullptr, m_testNameInteger));
    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(nullptr, nullptr));

    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigInteger(m_config, nullptr, 456));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigInteger(nullptr, m_testNameInteger, 456));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, nullptr, ConfigFileFormatToml));
    EXPECT_EQ(nullptr, OpenConfigFile(nullptr, ConfigFileFormatToml));

    EXPECT_STREQ(nullptr, ReadConfigString(m_config, nullptr));
    EXPECT_STREQ(nullptr, ReadConfigString(nullptr, m_testNameString));
    EXPECT_STREQ(nullptr, ReadConfigString(nullptr, nullptr));

    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameString, nullptr));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, nullptr, "replacementValue"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, nullptr, nullptr));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, m_testNameString, "replacementValue"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, m_testNameString, nullptr));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, nullptr, "replacemetValue"));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, nullptr, nullptr));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, OpenConfigFileSuccess)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_NE(nullptr, m_config);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, m_tomlData, ConfigFileFormatToml));
    EXPECT_NE(nullptr, m_config);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, ReadConfigStringSuccess)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    char* jsonValue = ReadConfigString(m_config, m_testNameStringJson);
    EXPECT_STREQ(m_testValueString, jsonValue);
    FreeConfigString(jsonValue);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, m_tomlData, ConfigFileFormatToml));
    char* tomlValue = ReadConfigString(m_config, m_testNameString);
    EXPECT_STREQ(m_testValueString, tomlValue);
    FreeConfigString(tomlValue);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, WriteConfigStringSuccess)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigString(m_config, m_testNameStringJson, "replacementValue"));
    char* jsonReplacementValue = ReadConfigString(m_config, m_testNameStringJson);
    EXPECT_STREQ("replacementValue", jsonReplacementValue);
    FreeConfigString(jsonReplacementValue);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));

    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, m_tomlData, ConfigFileFormatToml));
    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigString(m_config, m_testNameString, "replacementValue"));
    char* tomlReplacementValue = ReadConfigString(m_config, m_testNameString);
    EXPECT_STREQ("replacementValue", tomlReplacementValue);
    FreeConfigString(tomlReplacementValue);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}

TEST_F(ConfigFileUtilsTest, ReadNestedConfigStringJson)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonDataNested, ConfigFileFormatJson));
    char* jsonValue = ReadConfigString(m_config, m_nestedTestNameStringJson);
    EXPECT_STREQ(m_testValueString, jsonValue);
    FreeConfigString(jsonValue);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));
}

TEST_F(ConfigFileUtilsTest, WriteNestedConfigStringJson)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigString(m_config, m_nestedTestNameStringJson, "replacementValue"));
    char* jsonReplacementValue = ReadConfigString(m_config, m_nestedTestNameStringJson);
    EXPECT_STREQ("replacementValue", jsonReplacementValue);
    FreeConfigString(jsonReplacementValue);
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));
}

TEST_F(ConfigFileUtilsTest, ReadConfigIntegerSuccess)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_EQ(m_testValueInteger, ReadConfigInteger(m_config, m_testNameIntegerJson));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));
}

TEST_F(ConfigFileUtilsTest, WriteConfigIntegerSuccess)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigInteger(m_config, m_testNameIntegerJson, 456));
    EXPECT_EQ(456, ReadConfigInteger(m_config, m_testNameIntegerJson));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));
}

TEST_F(ConfigFileUtilsTest, ReadNestedConfigIntegerJson)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonDataNested, ConfigFileFormatJson));
    EXPECT_EQ(m_testValueInteger, ReadConfigInteger(m_config, m_nestedTestNameIntegerJson));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));
}

TEST_F(ConfigFileUtilsTest, WriteNestedConfigIntegerJson)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));
    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigInteger(m_config, m_nestedTestNameIntegerJson, 456));
    EXPECT_EQ(456, ReadConfigInteger(m_config, m_nestedTestNameIntegerJson));
    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));
}

TEST_F(ConfigFileUtilsTest, MultipleCallsJson)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_jsonPath, m_jsonData, ConfigFileFormatJson));

    EXPECT_STREQ(nullptr, ReadConfigString(m_config, "invalidName"));

    // Original string value persists after failed read.
    char* valueString = ReadConfigString(m_config, m_testNameStringJson);
    EXPECT_STREQ(m_testValueString, valueString);

    // Failed string write does not change value
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(nullptr, m_testNameStringJson, "invalidValue"));
    EXPECT_NE("invalidValue", std::string(ReadConfigString(m_config, m_testNameStringJson)));
    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, m_testNameStringJson, nullptr));
    EXPECT_NE(nullptr, ReadConfigString(m_config, m_testNameStringJson));

    // Original string value persists after multiple calls with invalid arguments.
    char* valueStringAfterFailedWrite = ReadConfigString(m_config, m_testNameStringJson);
    EXPECT_STREQ(valueString, valueStringAfterFailedWrite);

    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigString(m_config, m_testNameStringJson, "replacementValue"));

    // Replacement string value updates successfully after multiple calls with invalid arguments.
    char* replacementString = ReadConfigString(m_config, m_testNameStringJson);
    EXPECT_STREQ("replacementValue", replacementString);

    FreeConfigString(valueString);
    FreeConfigString(valueStringAfterFailedWrite);
    FreeConfigString(replacementString);

    EXPECT_EQ(READ_CONFIG_FAILURE, ReadConfigInteger(m_config, "invalidName"));

    // Origninal integer value persists after failed read.
    int valueInteger = ReadConfigInteger(m_config, m_testNameIntegerJson);
    EXPECT_EQ(m_testValueInteger, valueInteger);

    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigInteger(m_config, m_testNameIntegerJson, m_newValueInteger));

    // Replacement integer value updates successfully after multiple calls with invalid arguments.
    int replacementInteger = ReadConfigInteger(m_config, m_testNameIntegerJson);
    EXPECT_EQ(m_newValueInteger, replacementInteger);

    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_jsonPath));
}

TEST_F(ConfigFileUtilsTest, MultipleCallsToml)
{
    EXPECT_NE(nullptr, m_config = CreateTestHandleAndData(m_tomlPath, m_tomlData, ConfigFileFormatToml));

    EXPECT_STREQ(nullptr, ReadConfigString(m_config, "invalidName"));

    // Original string value persists after failed read.
    char* valueString = ReadConfigString(m_config, m_testNameString);
    EXPECT_STREQ(m_testValueString, valueString);

    EXPECT_EQ(WRITE_CONFIG_FAILURE, WriteConfigString(m_config, "invalidName", "replacementValue"));

    // Failed string write doesn't create new element.
    EXPECT_EQ(nullptr, ReadConfigString(m_config, "invalidName"));

    // Original string value persists after multiple calls with invalid arguments.
    char* valueStringAfterFailedWrite = ReadConfigString(m_config, m_testNameString);
    EXPECT_STREQ(valueString, valueStringAfterFailedWrite);

    EXPECT_EQ(WRITE_CONFIG_SUCCESS, WriteConfigString(m_config, m_testNameString, "replacementValue"));

    // Replacement string value updates successfully after multiple calls with invalid arguments.
    char* replacementString = ReadConfigString(m_config, m_testNameString);
    EXPECT_STREQ("replacementValue", replacementString);

    FreeConfigString(valueString);
    FreeConfigString(valueStringAfterFailedWrite);
    FreeConfigString(replacementString);

    EXPECT_TRUE(CleanupTestHandleAndData(m_config, m_tomlPath));
}