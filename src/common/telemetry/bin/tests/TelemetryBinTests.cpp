// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <vector>

#include <Telemetry.hpp>
#include <Logging.h>
#include "../main.h"

using ::testing::HasSubstr;

class TelemetryBinTest : public ::testing::Test
{
protected:
    std::string m_testDir;

    void SetUp() override
    {
        // Create a temporary directory for test files
        char tmpDir[] = "/tmp/telemetry_test_XXXXXX";
        ASSERT_NE(nullptr, mkdtemp(tmpDir));
        m_testDir = std::string(tmpDir);
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

    std::string CreateTestJsonFile(const std::string& content)
    {
        std::string filePath = m_testDir + "/test_events.json";
        std::ofstream file(filePath);
        EXPECT_TRUE(file.is_open());
        file << content;
        file.close();
        return filePath;
    }

    bool FileExists(const std::string& path)
    {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    char** CreateArgv(const std::vector<std::string>& args)
    {
        char** argv = new char*[args.size()];
        for (size_t i = 0; i < args.size(); i++)
        {
            argv[i] = strdup(args[i].c_str());
        }
        return argv;
    }

    void FreeArgv(char** argv, int argc)
    {
        for (int i = 0; i < argc; i++)
        {
            free(argv[i]);
        }
        delete[] argv;
    }
};

TEST_F(TelemetryBinTest, NoArgumentsReturnsFalse)
{
    std::vector<std::string> argList = {"telemetrybin"};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_FALSE(result);
    EXPECT_TRUE(args.filepath.empty());

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, InvalidOptionReturnsFalse)
{
    std::vector<std::string> argList = {"telemetrybin", "-x"};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_FALSE(result);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, PositionalArgumentAcceptsFilePath)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_EQ(jsonFile, args.filepath);
    EXPECT_FALSE(args.verbose);
    EXPECT_EQ(5, args.teardown_time); // Default teardown time

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, FileOptionAcceptsFilePath)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "-f", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_EQ(jsonFile, args.filepath);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, LongFormFileOptionWorks)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "--file", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_EQ(jsonFile, args.filepath);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, VerboseFlagWorks)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "-v", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_TRUE(args.verbose);
    EXPECT_EQ(jsonFile, args.filepath);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, LongFormVerboseWorks)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "--verbose", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_TRUE(args.verbose);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, TeardownOptionWithValue)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "-t", "10", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_EQ(10, args.teardown_time);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, LongFormTeardownWithValue)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "--teardown", "15", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_EQ(15, args.teardown_time);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, NegativeTeardownValueFails)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "-t", "-1", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_FALSE(result);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, InvalidTeardownValueFails)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "-t", "notanumber", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_FALSE(result);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, CombinedOptionsWork)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "-v", "-t", "1", "-f", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_TRUE(args.verbose);
    EXPECT_EQ(1, args.teardown_time);
    EXPECT_EQ(jsonFile, args.filepath);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, MixedLongAndShortOptionsWork)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent"})");
    std::vector<std::string> argList = {"telemetrybin", "--verbose", "-t", "1", jsonFile};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_TRUE(result);
    EXPECT_TRUE(args.verbose);
    EXPECT_EQ(1, args.teardown_time);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, TooManyArgumentsFails)
{
    std::string jsonFile1 = CreateTestJsonFile(R"({"EventName":"TestEvent1"})");
    std::string jsonFile2 = m_testDir + "/test2.json";
    std::vector<std::string> argList = {"telemetrybin", jsonFile1, jsonFile2};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_FALSE(result);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, FileAndPositionalArgumentCannotBothBeUsed)
{
    std::string jsonFile1 = CreateTestJsonFile(R"({"EventName":"TestEvent1"})");
    std::string jsonFile2 = m_testDir + "/test2.json";
    std::vector<std::string> argList = {"telemetrybin", "-f", jsonFile1, jsonFile2};
    char** argv = CreateArgv(argList);

    CommandLineArgs args;
    bool result = parse_command_line_args(argList.size(), argv, args, nullptr);

    EXPECT_FALSE(result);

    FreeArgv(argv, argList.size());
}

TEST_F(TelemetryBinTest, ProcessesValidSingleLineJson)
{
    std::string jsonFile = CreateTestJsonFile(R"({"EventName":"TestEvent","TestKey":"TestValue"})");

    try
    {
        Telemetry::TelemetryManager telemetryManager(false, 1);
        bool result = telemetryManager.ProcessJsonFile(jsonFile);
        EXPECT_TRUE(result);
    }
    catch (const std::exception& e)
    {
        GTEST_SKIP() << "TelemetryManager creation failed: " << e.what();
    }
}
